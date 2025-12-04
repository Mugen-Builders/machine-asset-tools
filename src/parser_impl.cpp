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
        printf("DEBUG cmt_abi_get_bytes_packed error begin: %p, end: %p\n", start->begin, start->end);
        throw CmaException("Invalid buffer pointer range", -ENOBUFS);
    }
    *n = cmt_buf_length(start);
    *data = start->begin;
}

auto cma_abi_get_bytes(const cmt_buf_t *me, size_t n, uint8_t *out) -> void {
    if (me == nullptr) {
        throw CmaException("Invalid bytes buffer pointer", -ENOBUFS);
    }
    if (out == nullptr) {
        throw CmaException("Invalid out pointer", -ENOBUFS);
    }
    for (size_t i = 0; i < n; ++i) {
        if (me->begin + i >= me->end) {
            throw CmaException("Invalid buffer pointer range", -ENOBUFS);
        }
        out[i] = me->begin[i];
    }
}

auto cma_abi_get_address_packed(cmt_buf_t *me, cma_abi_address_t *address) -> void {
    cmt_buf_t x[1];
    cma_abi_get_bytes(me, CMT_ABI_ADDRESS_LENGTH, address->data);
    cmt_buf_split(me, CMT_ABI_ADDRESS_LENGTH, x, me);
}

} // namespace

/*
 * Decode advance functions
 */

void cma_parser_decode_ether_deposit(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) {
    cmt_buf_t data_buf[1];
    cmt_buf_init(data_buf, input.payload.length, input.payload.data);
    cma_abi_get_address_packed(data_buf, &parser_input.ether_deposit.sender);
    int rc = cmt_abi_get_uint256(data_buf, &parser_input.ether_deposit.amount);
    if (rc) {
        throw CmaException("Error getting amount", rc);
    }
    cma_abi_get_bytes_packed(data_buf, &parser_input.ether_deposit.exec_layer_data.length,
        &parser_input.ether_deposit.exec_layer_data.data);
    parser_input.type = CMA_PARSER_INPUT_TYPE_ETHER_DEPOSIT;
}

void cma_parser_decode_erc20_deposit(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) {
    cmt_buf_t data_buf[1];
    cmt_buf_init(data_buf, input.payload.length, input.payload.data);
    cma_abi_get_address_packed(data_buf, &parser_input.erc20_deposit.sender);
    cma_abi_get_address_packed(data_buf, &parser_input.erc20_deposit.token);
    int rc = cmt_abi_get_uint256(data_buf, &parser_input.erc20_deposit.amount);
    if (rc) {
        throw CmaException("Error getting amount", rc);
    }
    cma_abi_get_bytes_packed(data_buf, &parser_input.erc20_deposit.exec_layer_data.length,
        &parser_input.erc20_deposit.exec_layer_data.data);
    parser_input.type = CMA_PARSER_INPUT_TYPE_ERC20_DEPOSIT;
}

void cma_parser_decode_ether_withdrawal(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) {
    cmt_buf_t data_buf[1];
    cmt_buf_init(data_buf, input.payload.length, input.payload.data);
    cmt_buf_t frame[1];
    cmt_buf_t of[1];

    int rc = 0;
    rc = cmt_abi_check_funsel(data_buf, WITHDRAW_ETHER);
    if (rc) {
        throw CmaException("Invalid funsel", rc);
    }

    rc = cmt_abi_mark_frame(data_buf, frame);
    if (rc) {
        throw CmaException("Error marking frame", rc);
    }
    rc = cmt_abi_get_uint256(data_buf, &parser_input.ether_withdrawal.amount);
    if (rc) {
        throw CmaException("Error getting amount", rc);
    }
    rc = cmt_abi_get_bytes_s(data_buf, of);
    if (rc) {
        throw CmaException("Error getting bytes offset", rc);
    }
    rc = cmt_abi_get_bytes_d(frame, of, &parser_input.ether_withdrawal.exec_layer_data.length,
        &parser_input.ether_withdrawal.exec_layer_data.data);
    if (rc) {
        throw CmaException("Error getting bytes", rc);
    }
    parser_input.type = CMA_PARSER_INPUT_TYPE_ETHER_WITHDRAWAL;
}

void cma_parser_decode_erc20_withdrawal(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) {
    cmt_buf_t data_buf[1];
    cmt_buf_init(data_buf, input.payload.length, input.payload.data);
    cmt_buf_t frame[1];
    cmt_buf_t of[1];

    int rc = 0;
    rc = cmt_abi_check_funsel(data_buf, WITHDRAW_ERC20);
    if (rc) {
        throw CmaException("Invalid funsel", rc);
    }

    rc = cmt_abi_mark_frame(data_buf, frame);
    if (rc) {
        throw CmaException("Error marking frame", rc);
    }
    rc = cmt_abi_get_address(data_buf, &parser_input.erc20_withdrawal.token);
    if (rc) {
        throw CmaException("Error getting token", rc);
    }
    rc = cmt_abi_get_uint256(data_buf, &parser_input.erc20_withdrawal.amount);
    if (rc) {
        throw CmaException("Error getting amount", rc);
    }
    rc = cmt_abi_get_bytes_s(data_buf, of);
    if (rc) {
        throw CmaException("Error getting bytes offset", rc);
    }
    rc = cmt_abi_get_bytes_d(frame, of, &parser_input.erc20_withdrawal.exec_layer_data.length,
        &parser_input.erc20_withdrawal.exec_layer_data.data);
    if (rc) {
        throw CmaException("Error getting bytes", rc);
    }
    parser_input.type = CMA_PARSER_INPUT_TYPE_ERC20_WITHDRAWAL;
}

void cma_parser_decode_ether_transfer(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) {
    cmt_buf_t data_buf[1];
    cmt_buf_init(data_buf, input.payload.length, input.payload.data);
    cmt_buf_t frame[1];
    cmt_buf_t of[1];

    int rc = 0;
    rc = cmt_abi_check_funsel(data_buf, TRANSFER_ETHER);
    if (rc) {
        throw CmaException("Invalid funsel", rc);
    }

    rc = cmt_abi_mark_frame(data_buf, frame);
    if (rc) {
        throw CmaException("Error marking frame", rc);
    }
    rc = cmt_abi_get_uint256(data_buf, &parser_input.ether_transfer.receiver);
    if (rc) {
        throw CmaException("Error getting receiver", rc);
    }
    rc = cmt_abi_get_uint256(data_buf, &parser_input.ether_transfer.amount);
    if (rc) {
        throw CmaException("Error getting amount", rc);
    }
    rc = cmt_abi_get_bytes_s(data_buf, of);
    if (rc) {
        throw CmaException("Error getting bytes offset", rc);
    }
    rc = cmt_abi_get_bytes_d(frame, of, &parser_input.ether_transfer.exec_layer_data.length,
        &parser_input.ether_transfer.exec_layer_data.data);
    if (rc) {
        throw CmaException("Error getting bytes", rc);
    }
    parser_input.type = CMA_PARSER_INPUT_TYPE_ETHER_TRANSFER;
}

/*
 * Decode inspect functions
 */

/*
 * Encode voucher functions
 */
