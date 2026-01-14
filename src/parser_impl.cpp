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

auto cma_abi_get_address_packed(cmt_buf_t *buf, cma_abi_address_t *address) -> void {
    cmt_buf_t lbuf = {};
    if (cmt_buf_length(buf) < CMA_ABI_ADDRESS_LENGTH) {
        throw CmaException("Invalid buffer length", -ENOBUFS);
    }
    cma_abi_get_bytes(buf, CMA_ABI_ADDRESS_LENGTH, std::begin(address->data));
    cmt_buf_split(buf, CMA_ABI_ADDRESS_LENGTH, &lbuf, buf);
}

int cma_abi_peek_uint256_list_d(const cmt_buf_t *start, cmt_buf_t *offset_buf, cmt_buf_t *bytes) {
    uint64_t offset = 0;
    uint64_t size = 0;
    int err = cmt_abi_get_uint(offset_buf, sizeof(offset), &offset);
    if (err) {
        return err;
    }

    /* from the beginning, after funsel */
    cmt_buf_t it_data = {start->begin + offset, start->end};
    cmt_buf_t *it = &it_data;
    err = cmt_abi_get_uint(it, sizeof(size), &size);
    if (err) {
        return err;
    }

    // fix siz in number of uint256s
    size = size * CMA_ABI_U256_LENGTH;
    if (cmt_buf_length(it) < size) {
        return -ENOBUFS;
    }
    return cmt_buf_split(it, size, bytes, it);
}

int cma_abi_get_uint256_list_d(const cmt_buf_t *start, cmt_buf_t *offset_buf, size_t *n, cma_abi_u256_data **uints) {
    cmt_buf_t bytes_data = {};
    cmt_buf_t *bytes = &bytes_data;
    int rc = cma_abi_peek_uint256_list_d(start, offset_buf, bytes);
    if (rc) {
        return rc;
    }
    *n = cmt_buf_length(bytes)/CMA_ABI_U256_LENGTH;
    *uints = reinterpret_cast<cma_abi_u256_data *>(bytes->begin);
    return 0;
}

int cma_abi_reserve_uints_d(cmt_buf_t *me, cmt_buf_t *of, size_t n, cmt_buf_t *out, const void *start) {
    int err = 0;
    cmt_buf_t tmp_data = {};
    cmt_buf_t *tmp = &tmp_data;
    cmt_buf_t uints_data = {};
    cmt_buf_t *uints = &uints_data;
    size_t n32 = n * CMT_ABI_U256_LENGTH;

    err = cmt_buf_split(me, CMT_ABI_U256_LENGTH, uints, tmp);
    if (err) {
        return err;
    }
    err = cmt_buf_split(tmp, n32, out, tmp);
    if (err) {
        return err;
    }

    size_t offset = uints->begin - (uint8_t *) start;
    err = cmt_abi_encode_uint(sizeof(offset), &offset, of->begin);
    if (err) {
        return err;
    }
    err = cmt_abi_encode_uint(sizeof(n), &n, uints->begin);
    if (err) {
        return err;
    }

    *me = *tmp; // commit the buffer changes
    return 0;
}

int cma_abi_put_uint256_list_d(cmt_buf_t *me, cmt_buf_t *offset, const cmt_buf_t *frame, const cma_abi_u256_list_t *payload) {
    cmt_buf_t res_data = {};
    cmt_buf_t *res = &res_data;
    int rc = cma_abi_reserve_uints_d(me, offset, payload->length, res, frame->begin);
    if (rc) {
        return rc;
    }

    std::span payload_span(reinterpret_cast<uint8_t *>(payload->data), payload->length * CMA_ABI_U256_LENGTH);
    std::ignore = std::copy_n(payload_span.begin(), payload_span.size(), res->begin);
    return 0;
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
    if (cmt_abi_check_funsel(buf, convert_to_cmt_funsel(WITHDRAW_ERC721)) == 0) {
        cma_parser_decode_erc721_withdrawal(input, parser_input);
        return;
    }
    if (cmt_abi_check_funsel(buf, convert_to_cmt_funsel(WITHDRAW_ERC1155_SINGLE)) == 0) {
        cma_parser_decode_erc1155_single_withdrawal(input, parser_input);
        return;
    }
    if (cmt_abi_check_funsel(buf, convert_to_cmt_funsel(WITHDRAW_ERC1155_BATCH)) == 0) {
        cma_parser_decode_erc1155_batch_withdrawal(input, parser_input);
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
    if (cmt_abi_check_funsel(buf, convert_to_cmt_funsel(TRANSFER_ERC721)) == 0) {
        cma_parser_decode_erc721_transfer(input, parser_input);
        return;
    }
    if (cmt_abi_check_funsel(buf, convert_to_cmt_funsel(TRANSFER_ERC1155_SINGLE)) == 0) {
        cma_parser_decode_erc1155_single_transfer(input, parser_input);
        return;
    }
    if (cmt_abi_check_funsel(buf, convert_to_cmt_funsel(TRANSFER_ERC1155_BATCH)) == 0) {
        cma_parser_decode_erc1155_batch_transfer(input, parser_input);
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

void cma_parser_decode_erc721_deposit(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) {
    cmt_buf_t buf_data = {};
    cmt_buf_t *buf = &buf_data;
    cmt_buf_init(buf, input.payload.length, input.payload.data);
    cma_abi_get_address_packed(buf, &parser_input.erc721_deposit.token);
    cma_abi_get_address_packed(buf, &parser_input.erc721_deposit.sender);
    int err = cmt_abi_get_uint256(buf, &parser_input.erc721_deposit.token_id);
    if (err != 0) {
        throw CmaException("Error getting token id", err);
    }

    // get data
    cmt_buf_t buf_payload_data = {};
    cmt_buf_t *buf_payload = &buf_payload_data;
    cmt_buf_t frame_data = {};
    cmt_buf_t *frame = &frame_data;
    cmt_buf_t offset_base_data = {};
    cmt_buf_t *offset_base = &offset_base_data;
    cmt_buf_t offset_exec_data = {};
    cmt_buf_t *offset_exec = &offset_exec_data;

    cmt_buf_init(buf_payload, cmt_buf_length(buf), buf->begin);
    err = cmt_abi_mark_frame(buf_payload, frame);
    if (err != 0) {
        throw CmaException("Error marking frame", err);
    }

    err = cmt_abi_get_bytes_s(buf_payload, offset_base);
    if (err != 0) {
        throw CmaException("Error getting bytes base offset", err);
    }
    err = cmt_abi_get_bytes_s(buf_payload, offset_exec);
    if (err != 0) {
        throw CmaException("Error getting bytes exec offset", err);
    }
    err = cmt_abi_get_bytes_d(frame, offset_base, &parser_input.erc721_deposit.base_layer_data.length,
        &parser_input.erc721_deposit.base_layer_data.data);
    if (err != 0) {
        throw CmaException("Error getting base bytes", err);
    }
    err = cmt_abi_get_bytes_d(frame, offset_exec, &parser_input.erc721_deposit.exec_layer_data.length,
        &parser_input.erc721_deposit.exec_layer_data.data);
    if (err != 0) {
        throw CmaException("Error getting exec bytes", err);
    }

    parser_input.type = CMA_PARSER_INPUT_TYPE_ERC721_DEPOSIT;
}

void cma_parser_decode_erc1155_single_deposit(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) {
    cmt_buf_t buf_data = {};
    cmt_buf_t *buf = &buf_data;
    cmt_buf_init(buf, input.payload.length, input.payload.data);
    cma_abi_get_address_packed(buf, &parser_input.erc1155_single_deposit.token);
    cma_abi_get_address_packed(buf, &parser_input.erc1155_single_deposit.sender);
    int err = cmt_abi_get_uint256(buf, &parser_input.erc1155_single_deposit.token_id);
    if (err != 0) {
        throw CmaException("Error getting token id", err);
    }
    err = cmt_abi_get_uint256(buf, &parser_input.erc1155_single_deposit.amount);
    if (err != 0) {
        throw CmaException("Error getting amount", err);
    }

    // get data
    cmt_buf_t buf_payload_data = {};
    cmt_buf_t *buf_payload = &buf_payload_data;
    cmt_buf_t frame_data = {};
    cmt_buf_t *frame = &frame_data;
    cmt_buf_t offset_base_data = {};
    cmt_buf_t *offset_base = &offset_base_data;
    cmt_buf_t offset_exec_data = {};
    cmt_buf_t *offset_exec = &offset_exec_data;

    cmt_buf_init(buf_payload, cmt_buf_length(buf), buf->begin);
    err = cmt_abi_mark_frame(buf_payload, frame);
    if (err != 0) {
        throw CmaException("Error marking frame", err);
    }

    err = cmt_abi_get_bytes_s(buf_payload, offset_base);
    if (err != 0) {
        throw CmaException("Error getting bytes base offset", err);
    }
    err = cmt_abi_get_bytes_s(buf_payload, offset_exec);
    if (err != 0) {
        throw CmaException("Error getting bytes exec offset", err);
    }
    err = cmt_abi_get_bytes_d(frame, offset_base, &parser_input.erc1155_single_deposit.base_layer_data.length,
        &parser_input.erc1155_single_deposit.base_layer_data.data);
    if (err != 0) {
        throw CmaException("Error getting base bytes", err);
    }
    err = cmt_abi_get_bytes_d(frame, offset_exec, &parser_input.erc1155_single_deposit.exec_layer_data.length,
        &parser_input.erc1155_single_deposit.exec_layer_data.data);
    if (err != 0) {
        throw CmaException("Error getting exec bytes", err);
    }

    parser_input.type = CMA_PARSER_INPUT_TYPE_ERC1155_SINGLE_DEPOSIT;
}

void cma_parser_decode_erc1155_batch_deposit(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) {
    int err = 0;
    cmt_buf_t buf_data = {};
    cmt_buf_t *buf = &buf_data;
    cmt_buf_init(buf, input.payload.length, input.payload.data);
    cma_abi_get_address_packed(buf, &parser_input.erc1155_batch_deposit.token);
    cma_abi_get_address_packed(buf, &parser_input.erc1155_batch_deposit.sender);

    // get data field
    cmt_buf_t buf_payload_data = {};
    cmt_buf_t *buf_payload = &buf_payload_data;
    cmt_buf_t frame_data = {};
    cmt_buf_t *frame = &frame_data;
    cmt_buf_t offset_token_id_data = {};
    cmt_buf_t *offset_token_id = &offset_token_id_data;
    cmt_buf_t offset_amount_data = {};
    cmt_buf_t *offset_amount = &offset_amount_data;
    cmt_buf_t offset_base_data = {};
    cmt_buf_t *offset_base = &offset_base_data;
    cmt_buf_t offset_exec_data = {};
    cmt_buf_t *offset_exec = &offset_exec_data;

    cmt_buf_init(buf_payload, cmt_buf_length(buf), buf->begin);
    err = cmt_abi_mark_frame(buf_payload, frame);
    if (err != 0) {
        throw CmaException("Error marking frame", err);
    }

    err = cmt_abi_get_bytes_s(buf_payload, offset_token_id);
    if (err != 0) {
        throw CmaException("Error getting bytes token id offset", err);
    }
    err = cmt_abi_get_bytes_s(buf_payload, offset_amount);
    if (err != 0) {
        throw CmaException("Error getting bytes amount offset", err);
    }
    err = cmt_abi_get_bytes_s(buf_payload, offset_base);
    if (err != 0) {
        throw CmaException("Error getting bytes base offset", err);
    }
    err = cmt_abi_get_bytes_s(buf_payload, offset_exec);
    if (err != 0) {
        throw CmaException("Error getting bytes exec offset", err);
    }

    // get token ids and amounts
    err = cma_abi_get_uint256_list_d(frame, offset_token_id, &parser_input.erc1155_batch_deposit.token_ids.length,
        &parser_input.erc1155_batch_deposit.token_ids.data);
    if (err != 0) {
        throw CmaException("Error getting base bytes", err);
    }

    err = cma_abi_get_uint256_list_d(frame, offset_amount, &parser_input.erc1155_batch_deposit.amounts.length,
        &parser_input.erc1155_batch_deposit.amounts.data);
    if (err != 0) {
        throw CmaException("Error getting base bytes", err);
    }

    if (parser_input.erc1155_batch_deposit.token_ids.length !=
        parser_input.erc1155_batch_deposit.amounts.length) {
        throw CmaException("Mismatched token ids and amounts lengths", -EINVAL);
    }

    err = cmt_abi_get_bytes_d(frame, offset_base, &parser_input.erc1155_batch_deposit.base_layer_data.length,
        &parser_input.erc1155_batch_deposit.base_layer_data.data);
    if (err != 0) {
        throw CmaException("Error getting base bytes", err);
    }
    err = cmt_abi_get_bytes_d(frame, offset_exec, &parser_input.erc1155_batch_deposit.exec_layer_data.length,
        &parser_input.erc1155_batch_deposit.exec_layer_data.data);
    if (err != 0) {
        throw CmaException("Error getting exec bytes", err);
    }

    parser_input.type = CMA_PARSER_INPUT_TYPE_ERC1155_BATCH_DEPOSIT;
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

void cma_parser_decode_erc721_withdrawal(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) {
    cmt_buf_t buf_data = {};
    cmt_buf_t *buf = &buf_data;
    cmt_buf_init(buf, input.payload.length, input.payload.data);
    cmt_buf_t frame_data = {};
    cmt_buf_t *frame = &frame_data;
    cmt_buf_t offset_data = {};
    cmt_buf_t *offset = &offset_data;

    int err = 0;
    err = cmt_abi_check_funsel(buf, convert_to_cmt_funsel(WITHDRAW_ERC721));
    if (err != 0) {
        throw CmaException("Invalid funsel", err);
    }

    err = cmt_abi_mark_frame(buf, frame);
    if (err != 0) {
        throw CmaException("Error marking frame", err);
    }
    err = cmt_abi_get_address(buf, &parser_input.erc721_withdrawal.token);
    if (err != 0) {
        throw CmaException("Error getting token", err);
    }
    err = cmt_abi_get_uint256(buf, &parser_input.erc721_withdrawal.token_id);
    if (err != 0) {
        throw CmaException("Error getting token id", err);
    }
    err = cmt_abi_get_bytes_s(buf, offset);
    if (err != 0) {
        throw CmaException("Error getting bytes offset", err);
    }
    err = cmt_abi_get_bytes_d(frame, offset, &parser_input.erc721_withdrawal.exec_layer_data.length,
        &parser_input.erc721_withdrawal.exec_layer_data.data);
    if (err != 0) {
        throw CmaException("Error getting bytes", err);
    }
    parser_input.type = CMA_PARSER_INPUT_TYPE_ERC721_WITHDRAWAL;
}

void cma_parser_decode_erc1155_single_withdrawal(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) {
    cmt_buf_t buf_data = {};
    cmt_buf_t *buf = &buf_data;
    cmt_buf_init(buf, input.payload.length, input.payload.data);
    cmt_buf_t frame_data = {};
    cmt_buf_t *frame = &frame_data;
    cmt_buf_t offset_data = {};
    cmt_buf_t *offset = &offset_data;

    int err = 0;
    err = cmt_abi_check_funsel(buf, convert_to_cmt_funsel(WITHDRAW_ERC1155_SINGLE));
    if (err != 0) {
        throw CmaException("Invalid funsel", err);
    }

    err = cmt_abi_mark_frame(buf, frame);
    if (err != 0) {
        throw CmaException("Error marking frame", err);
    }
    err = cmt_abi_get_address(buf, &parser_input.erc1155_single_withdrawal.token);
    if (err != 0) {
        throw CmaException("Error getting token", err);
    }
    err = cmt_abi_get_uint256(buf, &parser_input.erc1155_single_withdrawal.token_id);
    if (err != 0) {
        throw CmaException("Error getting token id", err);
    }
    err = cmt_abi_get_uint256(buf, &parser_input.erc1155_single_withdrawal.amount);
    if (err != 0) {
        throw CmaException("Error getting amount", err);
    }
    err = cmt_abi_get_bytes_s(buf, offset);
    if (err != 0) {
        throw CmaException("Error getting bytes offset", err);
    }
    err = cmt_abi_get_bytes_d(frame, offset, &parser_input.erc1155_single_withdrawal.exec_layer_data.length,
        &parser_input.erc1155_single_withdrawal.exec_layer_data.data);
    if (err != 0) {
        throw CmaException("Error getting bytes", err);
    }
    parser_input.type = CMA_PARSER_INPUT_TYPE_ERC1155_SINGLE_WITHDRAWAL;
}

void cma_parser_decode_erc1155_batch_withdrawal(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) {
    cmt_buf_t buf_data = {};
    cmt_buf_t *buf = &buf_data;
    cmt_buf_init(buf, input.payload.length, input.payload.data);
    cmt_buf_t frame_data = {};
    cmt_buf_t *frame = &frame_data;
    cmt_buf_t offset_token_id_data = {};
    cmt_buf_t *offset_token_id = &offset_token_id_data;
    cmt_buf_t offset_amount_data = {};
    cmt_buf_t *offset_amount = &offset_amount_data;
    cmt_buf_t offset_exec_data = {};
    cmt_buf_t *offset_exec = &offset_exec_data;

    int err = 0;
    err = cmt_abi_check_funsel(buf, convert_to_cmt_funsel(WITHDRAW_ERC1155_BATCH));
    if (err != 0) {
        throw CmaException("Invalid funsel", err);
    }

    err = cmt_abi_mark_frame(buf, frame);
    if (err != 0) {
        throw CmaException("Error marking frame", err);
    }
    err = cmt_abi_get_address(buf, &parser_input.erc1155_batch_withdrawal.token);
    if (err != 0) {
        throw CmaException("Error getting token", err);
    }

    err = cmt_abi_get_bytes_s(buf, offset_token_id);
    if (err != 0) {
        throw CmaException("Error getting bytes token id offset", err);
    }
    err = cmt_abi_get_bytes_s(buf, offset_amount);
    if (err != 0) {
        throw CmaException("Error getting bytes amount offset", err);
    }
    err = cmt_abi_get_bytes_s(buf, offset_exec);
    if (err != 0) {
        throw CmaException("Error getting bytes exec offset", err);
    }

    // get token ids and amounts
    err = cma_abi_get_uint256_list_d(frame, offset_token_id, &parser_input.erc1155_batch_withdrawal.token_ids.length,
        &parser_input.erc1155_batch_withdrawal.token_ids.data);
    if (err != 0) {
        throw CmaException("Error getting base bytes", err);
    }

    err = cma_abi_get_uint256_list_d(frame, offset_amount, &parser_input.erc1155_batch_withdrawal.amounts.length,
        &parser_input.erc1155_batch_withdrawal.amounts.data);
    if (err != 0) {
        throw CmaException("Error getting base bytes", err);
    }

    if (parser_input.erc1155_batch_withdrawal.token_ids.length !=
        parser_input.erc1155_batch_withdrawal.amounts.length) {
        throw CmaException("Mismatched token ids and amounts lengths", -EINVAL);
    }

    err = cmt_abi_get_bytes_d(frame, offset_exec, &parser_input.erc1155_batch_withdrawal.exec_layer_data.length,
        &parser_input.erc1155_batch_withdrawal.exec_layer_data.data);
    if (err != 0) {
        throw CmaException("Error getting bytes", err);
    }
    parser_input.type = CMA_PARSER_INPUT_TYPE_ERC1155_BATCH_WITHDRAWAL;
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

void cma_parser_decode_erc721_transfer(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) {
    cmt_buf_t buf_data = {};
    cmt_buf_t *buf = &buf_data;
    cmt_buf_init(buf, input.payload.length, input.payload.data);
    cmt_buf_t frame_data = {};
    cmt_buf_t *frame = &frame_data;
    cmt_buf_t offset_data = {};
    cmt_buf_t *offset = &offset_data;

    int err = 0;
    err = cmt_abi_check_funsel(buf, convert_to_cmt_funsel(TRANSFER_ERC721));
    if (err != 0) {
        throw CmaException("Invalid funsel", err);
    }

    err = cmt_abi_mark_frame(buf, frame);
    if (err != 0) {
        throw CmaException("Error marking frame", err);
    }
    err = cmt_abi_get_address(buf, &parser_input.erc721_transfer.token);
    if (err != 0) {
        throw CmaException("Error getting token", err);
    }
    err = cmt_abi_get_uint256(buf, &parser_input.erc721_transfer.receiver);
    if (err != 0) {
        throw CmaException("Error getting receiver", err);
    }
    err = cmt_abi_get_uint256(buf, &parser_input.erc721_transfer.token_id);
    if (err != 0) {
        throw CmaException("Error getting token id", err);
    }
    err = cmt_abi_get_bytes_s(buf, offset);
    if (err != 0) {
        throw CmaException("Error getting bytes offset", err);
    }
    err = cmt_abi_get_bytes_d(frame, offset, &parser_input.erc721_transfer.exec_layer_data.length,
        &parser_input.erc721_transfer.exec_layer_data.data);
    if (err != 0) {
        throw CmaException("Error getting bytes", err);
    }
    parser_input.type = CMA_PARSER_INPUT_TYPE_ERC721_TRANSFER;
}

void cma_parser_decode_erc1155_single_transfer(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) {
    cmt_buf_t buf_data = {};
    cmt_buf_t *buf = &buf_data;
    cmt_buf_init(buf, input.payload.length, input.payload.data);
    cmt_buf_t frame_data = {};
    cmt_buf_t *frame = &frame_data;
    cmt_buf_t offset_data = {};
    cmt_buf_t *offset = &offset_data;

    int err = 0;
    err = cmt_abi_check_funsel(buf, convert_to_cmt_funsel(TRANSFER_ERC1155_SINGLE));
    if (err != 0) {
        throw CmaException("Invalid funsel", err);
    }

    err = cmt_abi_mark_frame(buf, frame);
    if (err != 0) {
        throw CmaException("Error marking frame", err);
    }
    err = cmt_abi_get_address(buf, &parser_input.erc1155_single_transfer.token);
    if (err != 0) {
        throw CmaException("Error getting token", err);
    }
    err = cmt_abi_get_uint256(buf, &parser_input.erc1155_single_transfer.receiver);
    if (err != 0) {
        throw CmaException("Error getting receiver", err);
    }
    err = cmt_abi_get_uint256(buf, &parser_input.erc1155_single_transfer.token_id);
    if (err != 0) {
        throw CmaException("Error getting token id", err);
    }
    err = cmt_abi_get_uint256(buf, &parser_input.erc1155_single_transfer.amount);
    if (err != 0) {
        throw CmaException("Error getting amount", err);
    }
    err = cmt_abi_get_bytes_s(buf, offset);
    if (err != 0) {
        throw CmaException("Error getting bytes offset", err);
    }
    err = cmt_abi_get_bytes_d(frame, offset, &parser_input.erc1155_single_transfer.exec_layer_data.length,
        &parser_input.erc1155_single_transfer.exec_layer_data.data);
    if (err != 0) {
        throw CmaException("Error getting bytes", err);
    }
    parser_input.type = CMA_PARSER_INPUT_TYPE_ERC1155_SINGLE_TRANSFER;
}

void cma_parser_decode_erc1155_batch_transfer(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) {
    cmt_buf_t buf_data = {};
    cmt_buf_t *buf = &buf_data;
    cmt_buf_init(buf, input.payload.length, input.payload.data);
    cmt_buf_t frame_data = {};
    cmt_buf_t *frame = &frame_data;
    cmt_buf_t offset_token_id_data = {};
    cmt_buf_t *offset_token_id = &offset_token_id_data;
    cmt_buf_t offset_amount_data = {};
    cmt_buf_t *offset_amount = &offset_amount_data;
    cmt_buf_t offset_exec_data = {};
    cmt_buf_t *offset_exec = &offset_exec_data;

    int err = 0;
    err = cmt_abi_check_funsel(buf, convert_to_cmt_funsel(TRANSFER_ERC1155_BATCH));
    if (err != 0) {
        throw CmaException("Invalid funsel", err);
    }

    err = cmt_abi_mark_frame(buf, frame);
    if (err != 0) {
        throw CmaException("Error marking frame", err);
    }
    err = cmt_abi_get_address(buf, &parser_input.erc1155_batch_transfer.token);
    if (err != 0) {
        throw CmaException("Error getting token", err);
    }
    err = cmt_abi_get_uint256(buf, &parser_input.erc1155_batch_transfer.receiver);
    if (err != 0) {
        throw CmaException("Error getting receiver", err);
    }

    err = cmt_abi_get_bytes_s(buf, offset_token_id);
    if (err != 0) {
        throw CmaException("Error getting bytes token id offset", err);
    }
    err = cmt_abi_get_bytes_s(buf, offset_amount);
    if (err != 0) {
        throw CmaException("Error getting bytes amount offset", err);
    }
    err = cmt_abi_get_bytes_s(buf, offset_exec);
    if (err != 0) {
        throw CmaException("Error getting bytes exec offset", err);
    }

    // get token ids and amounts
    err = cma_abi_get_uint256_list_d(frame, offset_token_id, &parser_input.erc1155_batch_transfer.token_ids.length,
        &parser_input.erc1155_batch_transfer.token_ids.data);
    if (err != 0) {
        throw CmaException("Error getting base bytes", err);
    }

    err = cma_abi_get_uint256_list_d(frame, offset_amount, &parser_input.erc1155_batch_transfer.amounts.length,
        &parser_input.erc1155_batch_transfer.amounts.data);
    if (err != 0) {
        throw CmaException("Error getting base bytes", err);
    }

    if (parser_input.erc1155_batch_transfer.token_ids.length !=
        parser_input.erc1155_batch_transfer.amounts.length) {
        throw CmaException("Mismatched token ids and amounts lengths", -EINVAL);
    }

    err = cmt_abi_get_bytes_d(frame, offset_exec, &parser_input.erc1155_batch_transfer.exec_layer_data.length,
        &parser_input.erc1155_batch_transfer.exec_layer_data.data);
    if (err != 0) {
        throw CmaException("Error getting bytes", err);
    }
    parser_input.type = CMA_PARSER_INPUT_TYPE_ERC1155_BATCH_TRANSFER;
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

    const std::span token_span(voucher_request.erc20.token.data);
    std::span voucher_address_span(voucher.address.data);
    std::span voucher_value_span(voucher.value.data);
    cmt_buf_t buf_data = {};
    cmt_buf_t *buf = &buf_data;
    cmt_buf_init(buf, voucher.payload.length, voucher.payload.data);
    int err;

    // 2: copy token address to voucher address
    std::ignore = std::copy_n(token_span.begin(), token_span.size(), voucher_address_span.begin());

    // 3: ensure no value
    std::fill_n(voucher_value_span.begin(), voucher_value_span.size(), (uint8_t) 0);

    // 4: copy funsel to payload
    err = cmt_abi_put_funsel(buf, convert_to_cmt_funsel(ERC20_TRANSFER_FUNCTION_SELECTOR_FUNSEL));
    if (err != 0) {
        throw CmaException("Error putting funsel", err);
    }

    // 5: copy receiver to payload
    err = cmt_abi_put_address(buf, &voucher_request.receiver);
    if (err != 0) {
        throw CmaException("Error putting receiver", err);
    }

    // 6: copy amount to payload
    err = cmt_abi_put_uint256(buf, &voucher_request.erc20.amount);
    if (err != 0) {
        throw CmaException("Error putting amount", err);
    }

}

auto cma_parser_encode_erc721_voucher(const cma_abi_address_t &app_address, const cma_parser_voucher_data_t &voucher_request, cma_voucher_t &voucher) -> void {
    // 1: check sizes of voucher struct
    if (voucher.payload.data == nullptr) {
        throw CmaException("Invalid voucher payload buffer", -ENOBUFS);
    }
    if (voucher.payload.length < CMA_PARSER_ERC721_VOUCHER_PAYLOAD_SIZE) {
        throw CmaException("Invalid voucher payload buffersize", -ENOBUFS);
    }

    const std::span token_span(voucher_request.erc721.token.data);
    std::span voucher_address_span(voucher.address.data);
    std::span voucher_value_span(voucher.value.data);
    cmt_buf_t buf_data = {};
    cmt_buf_t *buf = &buf_data;
    cmt_buf_init(buf, voucher.payload.length, voucher.payload.data);
    int err;

    // 2: copy token address to voucher address
    std::ignore = std::copy_n(token_span.begin(), token_span.size(), voucher_address_span.begin());

    // 3: ensure no value
    std::fill_n(voucher_value_span.begin(), voucher_value_span.size(), (uint8_t) 0);

    // 4: copy funsel to payload
    err = cmt_abi_put_funsel(buf, convert_to_cmt_funsel(ERC721_TRANSFER_FUNCTION_SELECTOR_FUNSEL));
    if (err != 0) {
        throw CmaException("Error putting funsel", err);
    }

    // 5: copy sender (app) to payload
    err = cmt_abi_put_address(buf, &app_address);
    if (err != 0) {
        throw CmaException("Error putting sender", err);
    }

    // 6: copy receiver to payload
    err = cmt_abi_put_address(buf, &voucher_request.receiver);
    if (err != 0) {
        throw CmaException("Error putting receiver", err);
    }

    // 7: copy token id to payload
    err = cmt_abi_put_uint256(buf, &voucher_request.erc721.token_id);
    if (err != 0) {
        throw CmaException("Error putting amount", err);
    }
}

auto cma_parser_encode_erc1155_single_voucher(const cma_abi_address_t &app_address, const cma_parser_voucher_data_t &voucher_request, cma_voucher_t &voucher) -> void {
    // 1: check sizes of voucher struct
    if (voucher.payload.data == nullptr) {
        throw CmaException("Invalid voucher payload buffer", -ENOBUFS);
    }
    if (voucher.payload.length < CMA_PARSER_ERC1155_SINGLE_VOUCHER_PAYLOAD_MIN_SIZE) {
        throw CmaException("Invalid voucher payload buffersize", -ENOBUFS);
    }

    const std::span token_span(voucher_request.erc1155_single.token.data);
    std::span voucher_address_span(voucher.address.data);
    std::span voucher_value_span(voucher.value.data);
    cmt_buf_t buf_data = {};
    cmt_buf_t *buf = &buf_data;
    cmt_buf_init(buf, voucher.payload.length, voucher.payload.data);
    cmt_buf_t frame_data = {};
    cmt_buf_t *frame = &frame_data;
    cmt_buf_t offset_data = {};
    cmt_buf_t *offset = &offset_data;
    int err;

    // 2: copy token address to voucher address
    std::ignore = std::copy_n(token_span.begin(), token_span.size(), voucher_address_span.begin());

    // 3: ensure no value
    std::fill_n(voucher_value_span.begin(), voucher_value_span.size(), (uint8_t) 0);

    // 4: copy funsel to payload
    err = cmt_abi_put_funsel(buf, convert_to_cmt_funsel(ERC1155_SINGLE_TRANSFER_FUNCTION_SELECTOR_FUNSEL));
    if (err != 0) {
        throw CmaException("Error putting funsel", err);
    }
    err = cmt_abi_mark_frame(buf, frame);
    if (err != 0) {
        throw CmaException("Error marking frame", err);
    }

    // 5: copy sender (app) to payload
    err = cmt_abi_put_address(buf, &app_address);
    if (err != 0) {
        throw CmaException("Error putting sender", err);
    }

    // 6: copy receiver to payload
    err = cmt_abi_put_address(buf, &voucher_request.receiver);
    if (err != 0) {
        throw CmaException("Error putting receiver", err);
    }

    // 7: copy token id to payload
    err = cmt_abi_put_uint256(buf, &voucher_request.erc1155_single.token_id);
    if (err != 0) {
        throw CmaException("Error putting amount", err);
    }

    // 8: copy amount to payload
    err = cmt_abi_put_uint256(buf, &voucher_request.erc1155_single.amount);
    if (err != 0) {
        throw CmaException("Error putting amount", err);
    }

    // 9: copy data offset to payload
    err = cmt_abi_put_bytes_s(buf, offset);
    if (err != 0) {
        throw CmaException("Error putting offset", err);
    }

    // 10: copy data to payload
    err = cmt_abi_put_bytes_d(buf, offset, frame, &voucher_request.erc1155_single.exec_layer_data);
    if (err != 0) {
        throw CmaException("Error putting data", err);
    }
}

auto cma_parser_encode_erc1155_batch_voucher(const cma_abi_address_t &app_address, const cma_parser_voucher_data_t &voucher_request, cma_voucher_t &voucher) -> void {
    // 1: check sizes of voucher struct
    if (voucher.payload.data == nullptr) {
        throw CmaException("Invalid voucher payload buffer", -ENOBUFS);
    }
    if (voucher.payload.length < CMA_PARSER_ERC1155_BATCH_VOUCHER_PAYLOAD_MIN_SIZE) {
        throw CmaException("Invalid voucher payload buffersize", -ENOBUFS);
    }
    if (voucher_request.erc1155_batch.token_ids.length !=
        voucher_request.erc1155_batch.amounts.length) {
        throw CmaException("Mismatched token ids and amounts lengths", -EINVAL);
    }

    const std::span token_span(voucher_request.erc1155_batch.token.data);
    std::span voucher_address_span(voucher.address.data);
    std::span voucher_value_span(voucher.value.data);
    cmt_buf_t buf_data = {};
    cmt_buf_t *buf = &buf_data;
    cmt_buf_init(buf, voucher.payload.length, voucher.payload.data);
    cmt_buf_t frame_data = {};
    cmt_buf_t *frame = &frame_data;
    cmt_buf_t offset_token_id_data = {};
    cmt_buf_t *offset_token_id = &offset_token_id_data;
    cmt_buf_t offset_amount_data = {};
    cmt_buf_t *offset_amount = &offset_amount_data;
    cmt_buf_t offset_data = {};
    cmt_buf_t *offset_exec = &offset_data;
    int err;

    // 2: copy token address to voucher address
    std::ignore = std::copy_n(token_span.begin(), token_span.size(), voucher_address_span.begin());

    // 3: ensure no value
    std::fill_n(voucher_value_span.begin(), voucher_value_span.size(), (uint8_t) 0);

    // 4: copy funsel to payload
    err = cmt_abi_put_funsel(buf, convert_to_cmt_funsel(ERC1155_BATCH_TRANSFER_FUNCTION_SELECTOR_FUNSEL));
    if (err != 0) {
        throw CmaException("Error putting funsel", err);
    }
    err = cmt_abi_mark_frame(buf, frame);
    if (err != 0) {
        throw CmaException("Error marking frame", err);
    }

    // 5: copy sender (app) to payload
    err = cmt_abi_put_address(buf, &app_address);
    if (err != 0) {
        throw CmaException("Error putting sender", err);
    }

    // 6: copy receiver to payload
    err = cmt_abi_put_address(buf, &voucher_request.receiver);
    if (err != 0) {
        throw CmaException("Error putting receiver", err);
    }

    // 7: set offsets on payload
    err = cmt_abi_put_bytes_s(buf, offset_token_id);
    if (err != 0) {
        throw CmaException("Error putting token id offset", err);
    }
    err = cmt_abi_put_bytes_s(buf, offset_amount);
    if (err != 0) {
        throw CmaException("Error putting amount offset", err);
    }
    err = cmt_abi_put_bytes_s(buf, offset_exec);
    if (err != 0) {
        throw CmaException("Error putting data offset", err);
    }

    // 8: copy token ids to payload
    err = cma_abi_put_uint256_list_d(buf, offset_token_id, frame, &voucher_request.erc1155_batch.token_ids);
    if (err != 0) {
        throw CmaException("Error putting token ids", err);
    }

    // 9: copy amounts to payload
    err = cma_abi_put_uint256_list_d(buf, offset_amount, frame, &voucher_request.erc1155_batch.amounts);
    if (err != 0) {
        throw CmaException("Error putting amounts", err);
    }

    // 10: copy data to payload
    err = cmt_abi_put_bytes_d(buf, offset_exec, frame, &voucher_request.erc1155_batch.exec_layer_data);
    if (err != 0) {
        throw CmaException("Error putting offset", err);
    }
}
