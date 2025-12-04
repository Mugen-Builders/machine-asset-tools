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
        case CMA_PARSER_INPUT_TYPE_ETHER_DEPOSIT:
            cma_parser_decode_ether_deposit(*input, *parser_input);
            break;
        case CMA_PARSER_INPUT_TYPE_ERC20_DEPOSIT:
            cma_parser_decode_erc20_deposit(*input, *parser_input);
            break;
        case CMA_PARSER_INPUT_TYPE_ETHER_WITHDRAWAL:
            cma_parser_decode_ether_withdrawal(*input, *parser_input);
            break;
        case CMA_PARSER_INPUT_TYPE_ERC20_WITHDRAWAL:
            cma_parser_decode_erc20_withdrawal(*input, *parser_input);
            break;
        case CMA_PARSER_INPUT_TYPE_ETHER_TRANSFER:
            cma_parser_decode_ether_transfer(*input, *parser_input);
            break;
        default:
            throw CmaException("Invalid type", -EINVAL);
    }
    return cma_parser_result_success();
} catch (...) {
    return cma_parser_result_failure();
}

auto cma_parser_get_last_error_message() -> const char * {
    return get_last_err_msg_storage().c_str();
}
