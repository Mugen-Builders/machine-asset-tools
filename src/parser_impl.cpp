#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <span>

extern "C" {
#include "libcma/parser.h"
#include "libcma/types.h"
#include <errno.h>
#include <libcmt/abi.h>
#include <libcmt/buf.h>
#include <libcmt/rollup.h>
}

#include "parser_impl.h"
#include "utils.h"

#define GET_BALANCE "ledger_getBalance"
#define GET_TOTAL_SUPPLY "ledger_getTotalSupply"

/*
 * Aux functions
 */

enum {
    CMA_ETHER_VOUCHER_SIZE = 0,
    CMA_ADDRESS_HEX_LENGTH = 2 + 2 * CMA_ABI_ADDRESS_LENGTH,
    CMA_MAX_ID_HEX_LENGTH = 2 + 2 * CMA_ABI_U256_LENGTH,
};

namespace {

union Uint32Bytes {
    uint32_t value;
    uint8_t bytes[sizeof(uint32_t)];
};

auto convert_to_cmt_funsel(uint32_t original_funsel) -> uint32_t {
    Uint32Bytes converter;
    converter.value = original_funsel;
    uint32_t inverted_funsel =
        CMT_ABI_FUNSEL(converter.bytes[3], converter.bytes[2], converter.bytes[1], converter.bytes[0]);
    return inverted_funsel;
}

auto cma_abi_get_bytes_packed(const cmt_buf_t *start, size_t *n, void **data) -> void {
    if (start == nullptr) {
        throw CmaException("Invalid bytes buffer pointer", -ENOBUFS);
    }
    if (n == nullptr) {
        throw CmaException("Invalid size pointer", -ENOBUFS);
    }
    if (data == nullptr) {
        throw CmaException("Invalid data pointer", -ENOBUFS);
    }
    if (start->begin > start->end) {
        throw CmaException("Invalid buffer pointer range", -ENOBUFS);
    }
    *n = cmt_buf_length(start);
    *data = start->begin;
}

auto cma_abi_get_bytes(const cmt_buf_t *buf, size_t n_bytes, uint8_t *out) -> void {
    if (buf == nullptr) {
        throw CmaException("Invalid bytes buffer pointer", -ENOBUFS);
    }
    if (out == nullptr) {
        throw CmaException("Invalid out pointer", -ENOBUFS);
    }
    if (cmt_buf_length(buf) < n_bytes) {
        throw CmaException("Invalid buffer length", -ENOBUFS);
    }
    const std::span<uint8_t> buf_span(buf->begin, buf->end);
    const std::span<uint8_t> out_span(out, n_bytes);
    for (size_t i = 0; i < n_bytes; ++i) {
        out_span[i] = buf_span[i];
    }
}

auto cma_abi_get_bool_packed(cmt_buf_t *buf, bool *val) -> void {
    cmt_buf_t lbuf = {};
    if (cmt_buf_length(buf) < CMA_ABI_BOOL_LENGTH) {
        throw CmaException("Invalid buffer length", -ENOBUFS);
    }
    uint8_t read_value;
    cma_abi_get_bytes(buf, CMA_ABI_BOOL_LENGTH, &read_value);
    *val = read_value != 0;
    cmt_buf_split(buf, CMA_ABI_BOOL_LENGTH, &lbuf, buf);
}

auto cma_abi_get_address_packed(cmt_buf_t *buf, cma_abi_address_t *address) -> void {
    cmt_buf_t lbuf = {};
    if (cmt_buf_length(buf) < CMA_ABI_ADDRESS_LENGTH) {
        throw CmaException("Invalid buffer length", -ENOBUFS);
    }
    cma_abi_get_bytes(buf, CMA_ABI_ADDRESS_LENGTH, std::begin(address->data));
    cmt_buf_split(buf, CMA_ABI_ADDRESS_LENGTH, &lbuf, buf);
}

auto cma_parser_decode_get_balance_json(const nlohmann::json &json_input, cma_parser_input_t &parser_input) -> void {
    if (!json_input.contains("params")) {
        throw CmaException("Missing params key", -EINVAL);
    }
    auto params = json_input.at("params").get<std::vector<std::string> >();

    if (params.size() == 0 || params.size() > 4) {
        throw CmaException("Invalid params size", -EINVAL);
    }

    std::string account_str = params[0];
    std::span account_span(parser_input.balance.account.data);
    size_t offset = 0;
    if (account_str.size() == CMA_ADDRESS_HEX_LENGTH) {
        offset = CMA_ABI_U256_LENGTH - CMA_ABI_ADDRESS_LENGTH;
        std::fill_n(account_span.begin(), offset, (uint8_t) 0);
    } else if (account_str.size() != CMA_MAX_ID_HEX_LENGTH) {
        throw CmaException("Invalid account address", -EINVAL);
    }
    if (account_str.substr(0, 2) != "0x") {
        throw CmaException("Invalid account address", -EINVAL);
    }

    for (size_t i = 2; i < account_str.length(); i += 2) {
        std::string byteString = account_str.substr(i, 2);
        uint8_t byteValue = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
        account_span[offset++] = byteValue;
    }

    parser_input.type = CMA_PARSER_INPUT_TYPE_BALANCE_ACCOUNT;

    if (params.size() == 1) {
        return;
    }

    std::string token_str = params[1];
    std::span token_span(parser_input.balance.token.data);
    if (token_str.size() != CMA_ADDRESS_HEX_LENGTH) {
        throw CmaException("Invalid token address", -EINVAL);
    }
    if (token_str.substr(0, 2) != "0x") {
        throw CmaException("Invalid token address", -EINVAL);
    }

    offset = 0;
    for (size_t i = 2; i < token_str.length(); i += 2) {
        std::string byteString = token_str.substr(i, 2);
        uint8_t byteValue = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
        token_span[offset++] = byteValue;
    }

    parser_input.type = CMA_PARSER_INPUT_TYPE_BALANCE_ACCOUNT_TOKEN_ADDRESS;

    if (params.size() == 2) {
        return;
    }

    std::string token_id_str = params[2];
    std::span token_id_span(parser_input.balance.token_id.data);
    if (token_id_str.size() > CMA_MAX_ID_HEX_LENGTH || token_id_str.size() % 2 != 0) {
        throw CmaException("Invalid token id", -EINVAL);
    }
    if (token_id_str.substr(0, 2) != "0x") {
        throw CmaException("Invalid token id", -EINVAL);
    }

    offset = 0;
    if (token_id_str.size() < CMA_MAX_ID_HEX_LENGTH) {
        offset = (CMA_MAX_ID_HEX_LENGTH - token_id_str.size()) / 2;
        std::fill_n(token_id_span.begin(), offset, (uint8_t) 0);
    }

    for (size_t i = 2; i < token_id_str.length(); i += 2) {
        std::string byteString = token_id_str.substr(i, 2);
        uint8_t byteValue = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
        token_id_span[offset++] = byteValue;
    }

    parser_input.type = CMA_PARSER_INPUT_TYPE_BALANCE_ACCOUNT_TOKEN_ADDRESS_ID;

    if (params.size() == 3 || parser_input.balance.exec_layer_data.data == nullptr ||
        parser_input.balance.exec_layer_data.length == 0) {
        parser_input.balance.exec_layer_data.data = nullptr;
        parser_input.balance.exec_layer_data.length = 0;
        return;
    }

    std::string exec_layer_data_str = params[3];
    int min_exec_data_length = std::min(exec_layer_data_str.size(), parser_input.balance.exec_layer_data.length);
    std::span exec_layer_data_span(static_cast<uint8_t *>(parser_input.balance.exec_layer_data.data),
        parser_input.balance.exec_layer_data.length);
    std::ignore = std::copy_n(exec_layer_data_str.begin(), min_exec_data_length, exec_layer_data_span.begin());
    parser_input.balance.exec_layer_data.length = min_exec_data_length;
}

auto cma_parser_decode_get_total_supply_json(const nlohmann::json &json_input, cma_parser_input_t &parser_input)
    -> void {
    parser_input.type = CMA_PARSER_INPUT_TYPE_SUPPLY;
    if (!json_input.contains("params")) {
        parser_input.supply.exec_layer_data.data = nullptr;
        parser_input.supply.exec_layer_data.length = 0;
        return;
    }
    auto params = json_input.at("params").get<std::vector<std::string> >();

    if (params.size() == 0) {
        parser_input.supply.exec_layer_data.data = nullptr;
        parser_input.supply.exec_layer_data.length = 0;
        return;
    }

    if (params.size() > 3) {
        throw CmaException("Invalid params size", -EINVAL);
    }

    std::string token_str = params[0];
    std::span token_span(parser_input.supply.token.data);
    if (token_str.size() != CMA_ADDRESS_HEX_LENGTH) {
        throw CmaException("Invalid token address", -EINVAL);
    }
    if (token_str.substr(0, 2) != "0x") {
        throw CmaException("Invalid token address", -EINVAL);
    }

    size_t offset = 0;
    for (size_t i = 2; i < token_str.length(); i += 2) {
        std::string byteString = token_str.substr(i, 2);
        uint8_t byteValue = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
        token_span[offset++] = byteValue;
    }

    parser_input.type = CMA_PARSER_INPUT_TYPE_SUPPLY_TOKEN_ADDRESS;

    if (params.size() == 1) {
        parser_input.supply.exec_layer_data.data = nullptr;
        parser_input.supply.exec_layer_data.length = 0;
        return;
    }

    std::string token_id_str = params[1];
    std::span token_id_span(parser_input.supply.token_id.data);
    if (token_id_str.size() > CMA_MAX_ID_HEX_LENGTH || token_id_str.size() % 2 != 0) {
        throw CmaException("Invalid token id", -EINVAL);
    }
    if (token_id_str.substr(0, 2) != "0x") {
        throw CmaException("Invalid token id", -EINVAL);
    }

    offset = 0;
    if (token_id_str.size() < CMA_MAX_ID_HEX_LENGTH) {
        offset = (CMA_MAX_ID_HEX_LENGTH - token_id_str.size()) / 2;
        std::fill_n(token_id_span.begin(), offset, (uint8_t) 0);
    }

    for (size_t i = 2; i < token_id_str.length(); i += 2) {
        std::string byteString = token_id_str.substr(i, 2);
        uint8_t byteValue = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
        token_id_span[offset++] = byteValue;
    }

    parser_input.type = CMA_PARSER_INPUT_TYPE_SUPPLY_TOKEN_ADDRESS_ID;

    if (params.size() == 2 || parser_input.supply.exec_layer_data.data == nullptr ||
        parser_input.supply.exec_layer_data.length == 0) {
        parser_input.supply.exec_layer_data.data = nullptr;
        parser_input.supply.exec_layer_data.length = 0;
        return;
    }

    std::string exec_layer_data_str = params[2];
    int min_exec_data_length = std::min(exec_layer_data_str.size(), parser_input.supply.exec_layer_data.length);
    std::span exec_layer_data_span(static_cast<uint8_t *>(parser_input.supply.exec_layer_data.data),
        parser_input.supply.exec_layer_data.length);
    std::ignore = std::copy_n(exec_layer_data_str.begin(), min_exec_data_length, exec_layer_data_span.begin());
    parser_input.supply.exec_layer_data.length = min_exec_data_length;
}

} // namespace

/*
 * Decode advance functions
 */

void cma_parser_decode_advance_auto(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) {
    cmt_buf_t buf_data = {};
    cmt_buf_t *buf = &buf_data;
    cmt_buf_init(buf, input.payload.length, input.payload.data);

    if (cmt_abi_check_funsel(buf, convert_to_cmt_funsel(WITHDRAW_ETHER)) == 0) {
        cma_parser_decode_ether_withdrawal(input, parser_input);
        return;
    }
    if (cmt_abi_check_funsel(buf, convert_to_cmt_funsel(WITHDRAW_ERC20)) == 0) {
        cma_parser_decode_erc20_withdrawal(input, parser_input);
        return;
    }
    if (cmt_abi_check_funsel(buf, convert_to_cmt_funsel(TRANSFER_ETHER)) == 0) {
        cma_parser_decode_ether_transfer(input, parser_input);
        return;
    }
    if (cmt_abi_check_funsel(buf, convert_to_cmt_funsel(TRANSFER_ERC20)) == 0) {
        cma_parser_decode_erc20_transfer(input, parser_input);
        return;
    }
    throw CmaException("Invalid funsel", -EINVAL);
}

void cma_parser_decode_ether_deposit(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) {
    cmt_buf_t buf_data = {};
    cmt_buf_t *buf = &buf_data;
    cmt_buf_init(buf, input.payload.length, input.payload.data);
    cma_abi_get_address_packed(buf, &parser_input.ether_deposit.sender);
    const int err = cmt_abi_get_uint256(buf, &parser_input.ether_deposit.amount);
    if (err != 0) {
        throw CmaException("Error getting amount", err);
    }
    cma_abi_get_bytes_packed(buf, &parser_input.ether_deposit.exec_layer_data.length,
        &parser_input.ether_deposit.exec_layer_data.data);
    parser_input.type = CMA_PARSER_INPUT_TYPE_ETHER_DEPOSIT;
}

void cma_parser_decode_erc20_deposit(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) {
    cmt_buf_t buf_data = {};
    cmt_buf_t *buf = &buf_data;
    cmt_buf_init(buf, input.payload.length, input.payload.data);
    cma_abi_get_bool_packed(buf, &parser_input.erc20_deposit.success);
    cma_abi_get_address_packed(buf, &parser_input.erc20_deposit.token);
    cma_abi_get_address_packed(buf, &parser_input.erc20_deposit.sender);
    const int err = cmt_abi_get_uint256(buf, &parser_input.erc20_deposit.amount);
    if (err != 0) {
        throw CmaException("Error getting amount", err);
    }
    cma_abi_get_bytes_packed(buf, &parser_input.erc20_deposit.exec_layer_data.length,
        &parser_input.erc20_deposit.exec_layer_data.data);
    parser_input.type = CMA_PARSER_INPUT_TYPE_ERC20_DEPOSIT;
}

void cma_parser_decode_ether_withdrawal(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) {
    cmt_buf_t buf_data = {};
    cmt_buf_t *buf = &buf_data;
    cmt_buf_init(buf, input.payload.length, input.payload.data);
    cmt_buf_t frame_data = {};
    cmt_buf_t *frame = &frame_data;
    cmt_buf_t offset_data = {};
    cmt_buf_t *offset = &offset_data;

    int err = 0;
    err = cmt_abi_check_funsel(buf, convert_to_cmt_funsel(WITHDRAW_ETHER));
    if (err != 0) {
        throw CmaException("Invalid funsel", err);
    }

    err = cmt_abi_mark_frame(buf, frame);
    if (err != 0) {
        throw CmaException("Error marking frame", err);
    }
    err = cmt_abi_get_uint256(buf, &parser_input.ether_withdrawal.amount);
    if (err != 0) {
        throw CmaException("Error getting amount", err);
    }
    err = cmt_abi_get_bytes_s(buf, offset);
    if (err != 0) {
        throw CmaException("Error getting bytes offset", err);
    }
    err = cmt_abi_get_bytes_d(frame, offset, &parser_input.ether_withdrawal.exec_layer_data.length,
        &parser_input.ether_withdrawal.exec_layer_data.data);
    if (err != 0) {
        throw CmaException("Error getting bytes", err);
    }
    parser_input.type = CMA_PARSER_INPUT_TYPE_ETHER_WITHDRAWAL;
}

void cma_parser_decode_erc20_withdrawal(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) {
    cmt_buf_t buf_data = {};
    cmt_buf_t *buf = &buf_data;
    cmt_buf_init(buf, input.payload.length, input.payload.data);
    cmt_buf_t frame_data = {};
    cmt_buf_t *frame = &frame_data;
    cmt_buf_t offset_data = {};
    cmt_buf_t *offset = &offset_data;

    int err = 0;
    err = cmt_abi_check_funsel(buf, convert_to_cmt_funsel(WITHDRAW_ERC20));
    if (err != 0) {
        throw CmaException("Invalid funsel", err);
    }

    err = cmt_abi_mark_frame(buf, frame);
    if (err != 0) {
        throw CmaException("Error marking frame", err);
    }
    err = cmt_abi_get_address(buf, &parser_input.erc20_withdrawal.token);
    if (err != 0) {
        throw CmaException("Error getting token", err);
    }
    err = cmt_abi_get_uint256(buf, &parser_input.erc20_withdrawal.amount);
    if (err != 0) {
        throw CmaException("Error getting amount", err);
    }
    err = cmt_abi_get_bytes_s(buf, offset);
    if (err != 0) {
        throw CmaException("Error getting bytes offset", err);
    }
    err = cmt_abi_get_bytes_d(frame, offset, &parser_input.erc20_withdrawal.exec_layer_data.length,
        &parser_input.erc20_withdrawal.exec_layer_data.data);
    if (err != 0) {
        throw CmaException("Error getting bytes", err);
    }
    parser_input.type = CMA_PARSER_INPUT_TYPE_ERC20_WITHDRAWAL;
}

void cma_parser_decode_ether_transfer(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) {
    cmt_buf_t buf_data = {};
    cmt_buf_t *buf = &buf_data;
    cmt_buf_init(buf, input.payload.length, input.payload.data);
    cmt_buf_t frame_data = {};
    cmt_buf_t *frame = &frame_data;
    cmt_buf_t offset_data = {};
    cmt_buf_t *offset = &offset_data;

    int err = 0;
    err = cmt_abi_check_funsel(buf, convert_to_cmt_funsel(TRANSFER_ETHER));
    if (err != 0) {
        throw CmaException("Invalid funsel", err);
    }

    err = cmt_abi_mark_frame(buf, frame);
    if (err != 0) {
        throw CmaException("Error marking frame", err);
    }
    err = cmt_abi_get_uint256(buf, &parser_input.ether_transfer.receiver);
    if (err != 0) {
        throw CmaException("Error getting receiver", err);
    }
    err = cmt_abi_get_uint256(buf, &parser_input.ether_transfer.amount);
    if (err != 0) {
        throw CmaException("Error getting amount", err);
    }
    err = cmt_abi_get_bytes_s(buf, offset);
    if (err != 0) {
        throw CmaException("Error getting bytes offset", err);
    }
    err = cmt_abi_get_bytes_d(frame, offset, &parser_input.ether_transfer.exec_layer_data.length,
        &parser_input.ether_transfer.exec_layer_data.data);
    if (err != 0) {
        throw CmaException("Error getting bytes", err);
    }
    parser_input.type = CMA_PARSER_INPUT_TYPE_ETHER_TRANSFER;
}

void cma_parser_decode_erc20_transfer(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) {
    cmt_buf_t buf_data = {};
    cmt_buf_t *buf = &buf_data;
    cmt_buf_init(buf, input.payload.length, input.payload.data);
    cmt_buf_t frame_data = {};
    cmt_buf_t *frame = &frame_data;
    cmt_buf_t offset_data = {};
    cmt_buf_t *offset = &offset_data;

    int err = 0;
    err = cmt_abi_check_funsel(buf, convert_to_cmt_funsel(TRANSFER_ERC20));
    if (err != 0) {
        throw CmaException("Invalid funsel", err);
    }

    err = cmt_abi_mark_frame(buf, frame);
    if (err != 0) {
        throw CmaException("Error marking frame", err);
    }
    err = cmt_abi_get_address(buf, &parser_input.erc20_transfer.token);
    if (err != 0) {
        throw CmaException("Error getting token", err);
    }
    err = cmt_abi_get_uint256(buf, &parser_input.erc20_transfer.receiver);
    if (err != 0) {
        throw CmaException("Error getting receiver", err);
    }
    err = cmt_abi_get_uint256(buf, &parser_input.erc20_transfer.amount);
    if (err != 0) {
        throw CmaException("Error getting amount", err);
    }
    err = cmt_abi_get_bytes_s(buf, offset);
    if (err != 0) {
        throw CmaException("Error getting bytes offset", err);
    }
    err = cmt_abi_get_bytes_d(frame, offset, &parser_input.erc20_transfer.exec_layer_data.length,
        &parser_input.erc20_transfer.exec_layer_data.data);
    if (err != 0) {
        throw CmaException("Error getting bytes", err);
    }
    parser_input.type = CMA_PARSER_INPUT_TYPE_ERC20_TRANSFER;
}

/*
 * Decode inspect functions
 */

auto cma_parser_decode_inspect_auto(const cmt_rollup_inspect_t &input, cma_parser_input_t &parser_input) -> void try {
    std::span payload_span(static_cast<uint8_t *>(input.payload.data), input.payload.length);
    std::string json_str(payload_span.begin(), payload_span.end());

    auto json_input = nlohmann::json::parse(json_str);

    if (!json_input.contains("method")) {
        throw CmaException("Missing method key", -EINVAL);
    }
    auto method = json_input.at("method").get<std::string>();

    if (method == GET_BALANCE) {
        cma_parser_decode_get_balance_json(json_input, parser_input);
    } else if (method == GET_TOTAL_SUPPLY) {
        cma_parser_decode_get_total_supply_json(json_input, parser_input);
    } else {
        throw CmaException("Invalid method", -EINVAL);
    }
} catch (nlohmann::json::parse_error &e) {
    throw CmaException(std::string("Error parsing input: ").append(e.what()), -EINVAL);
}

auto cma_parser_decode_get_balance(const cmt_rollup_inspect_t &input, cma_parser_input_t &parser_input) -> void try {
    std::span payload_span(static_cast<uint8_t *>(input.payload.data), input.payload.length);
    std::string json_str(payload_span.begin(), payload_span.end());

    auto json_input = nlohmann::json::parse(json_str);

    if (!json_input.contains("method")) {
        throw CmaException("Missing method key", -EINVAL);
    }
    if (json_input.at("method").get<std::string>() != GET_BALANCE) {
        throw CmaException("Invalid method", -EINVAL);
    }

    cma_parser_decode_get_balance_json(json_input, parser_input);
} catch (nlohmann::json::parse_error &e) {
    throw CmaException(std::string("Error parsing input: ").append(e.what()), -EINVAL);
}

auto cma_parser_decode_get_total_supply(const cmt_rollup_inspect_t &input, cma_parser_input_t &parser_input)
    -> void try {
    std::span payload_span(static_cast<uint8_t *>(input.payload.data), input.payload.length);
    std::string json_str(payload_span.begin(), payload_span.end());

    auto json_input = nlohmann::json::parse(json_str);

    if (!json_input.contains("method")) {
        throw CmaException("Missing method key", -EINVAL);
    }
    if (json_input.at("method").get<std::string>() != GET_TOTAL_SUPPLY) {
        throw CmaException("Invalid method", -EINVAL);
    }

    cma_parser_decode_get_total_supply_json(json_input, parser_input);
} catch (nlohmann::json::parse_error &e) {
    throw CmaException(std::string("Error parsing input: ").append(e.what()), -EINVAL);
}

/*
 * Encode voucher functions
 */

auto cma_parser_encode_ether_voucher(const cma_parser_voucher_data_t &voucher_request, cma_voucher_t &voucher) -> void {
    const std::span receiver_span(voucher_request.receiver.data);
    const std::span amount_span(voucher_request.ether.amount.data);
    std::span voucher_address_span(voucher.address.data);
    std::span voucher_value_span(voucher.value.data);
    // 1: check sizes of voucher struct (not necessary for ether)
    // 2: copy receiver to voucher address
    std::ignore = std::copy_n(receiver_span.begin(), receiver_span.size(), voucher_address_span.begin());
    // 3: copy amount to voucher value
    std::ignore = std::copy_n(amount_span.begin(), amount_span.size(), voucher_value_span.begin());
    // 4: ensure empty payload
    voucher.payload.length = 0;
    voucher.payload.data = nullptr;
}

auto cma_parser_encode_erc20_voucher(const cma_parser_voucher_data_t &voucher_request, cma_voucher_t &voucher) -> void {
    // 1: check sizes of voucher struct
    if (voucher.payload.data == nullptr) {
        throw CmaException("Invalid voucher payload buffer", -ENOBUFS);
    }
    if (voucher.payload.length < CMA_PARSER_ERC20_VOUCHER_PAYLOAD_SIZE) {
        throw CmaException("Invalid voucher payload buffersize", -ENOBUFS);
    }

    const std::span receiver_span(voucher_request.receiver.data);
    const std::span token_span(voucher_request.erc20.token.data);
    const std::span amount_span(voucher_request.erc20.amount.data);
    std::span voucher_address_span(voucher.address.data);
    std::span voucher_value_span(voucher.value.data);
    std::span payload_span(static_cast<uint8_t *>(voucher.payload.data), voucher.payload.length);

    // 2: copy token address to voucher address
    std::ignore = std::copy_n(token_span.begin(), token_span.size(), voucher_address_span.begin());

    // 3: ensure no value
    std::fill_n(voucher_value_span.begin(), voucher_value_span.size(), (uint8_t) 0);

    // 4: copy funsel to payload
    Uint32Bytes funsel;
    funsel.value = convert_to_cmt_funsel(ERC20_TRANSFER_FUNCTION_SELECTOR_FUNSEL);
    std::ignore = std::copy_n(std::begin(funsel.bytes), CMA_PARSER_SELECTOR_SIZE, payload_span.begin());
    payload_span = payload_span.subspan(CMA_PARSER_SELECTOR_SIZE);

    // 5: copy receiver to payload
    std::fill_n(payload_span.begin(), CMA_ABI_U256_LENGTH - CMA_ABI_ADDRESS_LENGTH, (uint8_t) 0);
    payload_span = payload_span.subspan(CMA_ABI_U256_LENGTH - CMA_ABI_ADDRESS_LENGTH);
    std::ignore = std::copy_n(receiver_span.begin(), CMA_ABI_ADDRESS_LENGTH, payload_span.begin());
    payload_span = payload_span.subspan(CMA_ABI_ADDRESS_LENGTH);

    // 6: copy amount to payload
    std::ignore = std::copy_n(amount_span.begin(), CMA_ABI_U256_LENGTH, payload_span.begin());
}
