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

namespace {

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
    if (cmt_buf_length(buf) < CMT_ABI_ADDRESS_LENGTH) {
        throw CmaException("Invalid buffer length", -ENOBUFS);
    }
    cma_abi_get_bytes(buf, CMT_ABI_ADDRESS_LENGTH, static_cast<uint8_t *>(address->data));
    cmt_buf_split(buf, CMT_ABI_ADDRESS_LENGTH, &lbuf, buf);
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
    err = cmt_abi_check_funsel(buf, WITHDRAW_ETHER);
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
    err = cmt_abi_check_funsel(buf, WITHDRAW_ERC20);
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
    err = cmt_abi_check_funsel(buf, TRANSFER_ETHER);
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
