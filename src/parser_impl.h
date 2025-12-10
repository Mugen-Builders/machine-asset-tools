#ifndef CMA_PARSER_IMPL_H
#define CMA_PARSER_IMPL_H

extern "C" {
#include "libcma/parser.h"
#include "libcma/types.h"
#include <libcmt/rollup.h>
}

// Decode functions
auto cma_parser_decode_advance_auto(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) -> void;

// Decode advance functions
auto cma_parser_decode_ether_deposit(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) -> void;
auto cma_parser_decode_erc20_deposit(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) -> void;
auto cma_parser_decode_erc721_deposit(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) -> void;
auto cma_parser_decode_erc1155_single_deposit(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input)
    -> void;
auto cma_parser_decode_erc1155_batch_deposit(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input)
    -> void;
auto cma_parser_decode_ether_withdrawal(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) -> void;
auto cma_parser_decode_erc20_withdrawal(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) -> void;
auto cma_parser_decode_erc721_withdrawal(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) -> void;
auto cma_parser_decode_erc1155_single_withdrawal(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input)
    -> void;
auto cma_parser_decode_erc1155_batch_withdrawal(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input)
    -> void;
auto cma_parser_decode_ether_transfer(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) -> void;
auto cma_parser_decode_erc20_transfer(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) -> void;
auto cma_parser_decode_erc721_transfer(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input) -> void;
auto cma_parser_decode_erc1155_single_transfer(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input)
    -> void;
auto cma_parser_decode_erc1155_batch_transfer(const cmt_rollup_advance_t &input, cma_parser_input_t &parser_input)
    -> void;

// Decode inspect functions
auto cma_parser_decode_inspect_auto(const cmt_rollup_inspect_t &input, cma_parser_input_t &parser_input) -> void;
auto cma_parser_decode_get_balance(const cmt_rollup_inspect_t &input, cma_parser_input_t &parser_input) -> void;
auto cma_parser_decode_get_total_supply(const cmt_rollup_inspect_t &input, cma_parser_input_t &parser_input) -> void;

// Encode voucher functions
auto cma_parser_encode_ether_voucher(const cma_parser_voucher_data_t &voucher_request, cma_voucher_t &voucher) -> void;
auto cma_parser_encode_erc20_voucher(const cma_parser_voucher_data_t &voucher_request, cma_voucher_t &voucher) -> void;
auto cma_parser_encode_erc721_voucher(const cma_abi_address_t &app_address,
    const cma_parser_voucher_data_t &voucher_request, cma_voucher_t &voucher) -> void;
auto cma_parser_encode_erc1155_single_voucher(const cma_abi_address_t &app_address,
    const cma_parser_voucher_data_t &voucher_request, cma_voucher_t &voucher) -> void;
auto cma_parser_encode_erc1155_batch_voucher(const cma_abi_address_t &app_address,
    const cma_parser_voucher_data_t &voucher_request, cma_voucher_t &voucher) -> void;

#endif // CMA_PARSER_IMPL_H
