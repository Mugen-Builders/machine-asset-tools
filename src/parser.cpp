#include <cerrno>
#include <exception>
#include <string> // for string class

extern "C" {
#include "libcma/parser.h"
#include <libcmt/rollup.h>
}

#include "parser_impl.h"
#include "utils.h"

/*
 * Exception Management
 */

namespace {

auto get_last_err_msg_storage() -> std::string & {
    static thread_local std::string last_err_msg;
    return last_err_msg;
}

auto cma_parser_result_failure() -> int try { throw; } catch (const std::exception &e) {
    try {
        get_last_err_msg_storage() = e.what();
        throw;
    } catch (const CmaException &ex) {
        return ex.code();
    } catch (const std::exception &e) {
        return CMA_PARSER_ERROR_EXCEPTION;
    }
} catch (...) {
    try {
        get_last_err_msg_storage() = std::string("unknown error");
    } catch (...) {
        // Failed to allocate string, last resort is to set an empty error.
        get_last_err_msg_storage().clear();
    }
    return CMA_PARSER_ERROR_UNKNOWN;
}

auto cma_parser_result_success() -> int {
    get_last_err_msg_storage().clear();
    return CMA_PARSER_SUCCESS;
}

} // namespace

/*
 * Parser Api
 */

auto cma_parser_decode_advance(cma_parser_input_type_t type, const cmt_rollup_advance_t *input,
    cma_parser_input_t *parser_input) -> int try {
    if (input == nullptr) {
        throw CmaException("Invalid input", -EINVAL);
    }
    if (parser_input == nullptr) {
        throw CmaException("Invalid parser input", -EINVAL);
    }
    switch (type) {
        case CMA_PARSER_INPUT_TYPE_AUTO:
            cma_parser_decode_advance_auto(*input, *parser_input);
            break;
        case CMA_PARSER_INPUT_TYPE_ETHER_DEPOSIT:
            cma_parser_decode_ether_deposit(*input, *parser_input);
            break;
        case CMA_PARSER_INPUT_TYPE_ERC20_DEPOSIT:
            cma_parser_decode_erc20_deposit(*input, *parser_input);
            break;
        case CMA_PARSER_INPUT_TYPE_ERC721_DEPOSIT:
            cma_parser_decode_erc721_deposit(*input, *parser_input);
            break;
        case CMA_PARSER_INPUT_TYPE_ERC1155_SINGLE_DEPOSIT:
            cma_parser_decode_erc1155_single_deposit(*input, *parser_input);
            break;
        case CMA_PARSER_INPUT_TYPE_ETHER_WITHDRAWAL:
            cma_parser_decode_ether_withdrawal(*input, *parser_input);
            break;
        case CMA_PARSER_INPUT_TYPE_ERC20_WITHDRAWAL:
            cma_parser_decode_erc20_withdrawal(*input, *parser_input);
            break;
        case CMA_PARSER_INPUT_TYPE_ERC721_WITHDRAWAL:
            cma_parser_decode_erc721_withdrawal(*input, *parser_input);
            break;
        case CMA_PARSER_INPUT_TYPE_ERC1155_SINGLE_WITHDRAWAL:
            cma_parser_decode_erc1155_single_withdrawal(*input, *parser_input);
            break;
        case CMA_PARSER_INPUT_TYPE_ETHER_TRANSFER:
            cma_parser_decode_ether_transfer(*input, *parser_input);
            break;
        case CMA_PARSER_INPUT_TYPE_ERC20_TRANSFER:
            cma_parser_decode_erc20_transfer(*input, *parser_input);
            break;
        case CMA_PARSER_INPUT_TYPE_ERC721_TRANSFER:
            cma_parser_decode_erc721_transfer(*input, *parser_input);
            break;
        case CMA_PARSER_INPUT_TYPE_ERC1155_SINGLE_TRANSFER:
            cma_parser_decode_erc1155_single_transfer(*input, *parser_input);
            break;
        default:
            throw CmaException("Invalid advance decode type", -EINVAL);
    }
    return cma_parser_result_success();
} catch (...) {
    return cma_parser_result_failure();
}

auto cma_parser_decode_inspect(cma_parser_input_type_t type, const cmt_rollup_inspect_t *input,
    cma_parser_input_t *parser_input) -> int try {
    if (input == nullptr) {
        throw CmaException("Invalid input", -EINVAL);
    }
    if (parser_input == nullptr) {
        throw CmaException("Invalid parser input", -EINVAL);
    }

    switch (type) {
        case CMA_PARSER_INPUT_TYPE_AUTO:
            cma_parser_decode_inspect_auto(*input, *parser_input);
            break;
        case CMA_PARSER_INPUT_TYPE_BALANCE:
            cma_parser_decode_get_balance(*input, *parser_input);
            break;
        case CMA_PARSER_INPUT_TYPE_SUPPLY:
            cma_parser_decode_get_total_supply(*input, *parser_input);
            break;
        default:
            throw CmaException("Invalid inspect decode type", -EINVAL);
    }
    return cma_parser_result_success();
} catch (...) {
    return cma_parser_result_failure();
}

auto cma_parser_encode_voucher(cma_parser_voucher_type_t type, const cma_abi_address_t *app_address,
    const cma_parser_voucher_data_t *voucher_request, cma_voucher_t *voucher) -> int try {
    if (voucher_request == nullptr) {
        throw CmaException("Invalid request", -EINVAL);
    }
    if (voucher == nullptr) {
        throw CmaException("Invalid voucher output", -EINVAL);
    }
    switch (type) {
        case CMA_PARSER_VOUCHER_TYPE_ETHER:
            cma_parser_encode_ether_voucher(*voucher_request, *voucher);
            break;
        case CMA_PARSER_VOUCHER_TYPE_ERC20:
            cma_parser_encode_erc20_voucher(*voucher_request, *voucher);
            break;
        case CMA_PARSER_VOUCHER_TYPE_ERC721:
            if (app_address == nullptr) {
                throw CmaException("Invalid app address", -EINVAL);
            }
            cma_parser_encode_erc721_voucher(*app_address, *voucher_request, *voucher);
            break;
        case CMA_PARSER_VOUCHER_TYPE_ERC1155_SINGLE:
            if (app_address == nullptr) {
                throw CmaException("Invalid app address", -EINVAL);
            }
            cma_parser_encode_erc1155_single_voucher(*app_address, *voucher_request, *voucher);
            break;
        // case CMA_PARSER_VOUCHER_TYPE_ERC1155_BATCH:
        //     if (app_address == nullptr) {
        //         throw CmaException("Invalid app address", -EINVAL);
        //     }
        //     break;
        default:
            throw CmaException("Invalid voucher type", -EINVAL);
    }
    return cma_parser_result_success();
} catch (...) {
    return cma_parser_result_failure();
}

auto cma_parser_get_last_error_message() -> const char * {
    return get_last_err_msg_storage().c_str();
}
