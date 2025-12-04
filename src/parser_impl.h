#ifndef CMA_PARSER_IMPL_H
#define CMA_PARSER_IMPL_H

extern "C" {
#include "libcma/parser.h"
#include "libcma/types.h"
#include <libcmt/rollup.h>
}

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
// Encode voucher functions

#endif // CMA_PARSER_IMPL_H
