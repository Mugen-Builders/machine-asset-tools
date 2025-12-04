#include <algorithm>
#include <cstddef>
#include <cstdint>
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

/*
 * Aux functions
 */

enum {
    CMA_ETHER_VOUCHER_SIZE = 0,
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
    cma_abi_get_bytes(buf, CMA_ABI_ADDRESS_LENGTH, static_cast<uint8_t *>(address->data));
    cmt_buf_split(buf, CMA_ABI_ADDRESS_LENGTH, &lbuf, buf);
}

} // namespace

/*
 * Decode advance functions
 */

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
    cma_abi_get_address_packed(buf, &parser_input.erc20_deposit.sender);
    cma_abi_get_address_packed(buf, &parser_input.erc20_deposit.token);
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

/*
 * Decode inspect functions
 */

/*
 * Encode voucher functions
 */

auto cma_parser_encode_ether_voucher(const cma_parser_voucher_data_t &voucher_request, cma_voucher_t &voucher) -> void {
    const std::span receiver_span(voucher_request.receiver.data);
    const std::span amount_span(voucher_request.ether_voucher_fields.amount.data);
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
    const std::span token_span(voucher_request.erc20_voucher_fields.token.data);
    const std::span amount_span(voucher_request.erc20_voucher_fields.amount.data);
    std::span voucher_address_span(voucher.address.data);
    std::span voucher_value_span(voucher.value.data);
    std::span payload_span(static_cast<uint8_t *>(voucher.payload.data), voucher.payload.length);

    // 2: copy token address to voucher address
    std::ignore = std::copy_n(token_span.begin(), token_span.size(), voucher_address_span.begin());

    // 3: ensure no value
    std::fill_n(voucher_value_span.begin(), voucher_value_span.size(), (uint8_t) 0);

    // 4: copy funsel to payload
    Uint32Bytes funsel;
    funsel.value = ERC20_TRANSFER_FUNCTION_SELECTOR_FUNSEL;
    std::ignore = std::copy_n(static_cast<uint8_t *>(funsel.bytes), CMA_PARSER_SELECTOR_SIZE, payload_span.begin());
    payload_span = payload_span.subspan(CMA_PARSER_SELECTOR_SIZE);

    // 5: copy receiver to payload
    std::fill_n(payload_span.begin(), CMA_ABI_U256_LENGTH - CMA_ABI_ADDRESS_LENGTH, (uint8_t) 0);
    payload_span = payload_span.subspan(CMA_ABI_U256_LENGTH - CMA_ABI_ADDRESS_LENGTH);
    std::ignore = std::copy_n(receiver_span.begin(), CMA_ABI_ADDRESS_LENGTH, payload_span.begin());
    payload_span = payload_span.subspan(CMA_ABI_ADDRESS_LENGTH);

    // 6: copy amount to payload
    std::ignore = std::copy_n(amount_span.begin(), CMA_ABI_U256_LENGTH, payload_span.begin());
}
