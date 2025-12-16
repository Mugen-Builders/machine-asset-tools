#include <cerrno>
#include <exception>
#include <string> // for string class

extern "C" {
#include "libcma/ledger.h"
#include "libcma/types.h"
}

#include "ledger_impl.h"
#include "utils.h"

/*
 * Exception Management
 */

namespace {

auto get_last_err_msg_storage() -> std::string & {
    static thread_local std::string last_err_msg;
    return last_err_msg;
}

auto cma_ledger_result_failure() -> int try { throw; } catch (const std::exception &e) {
    try {
        get_last_err_msg_storage() = e.what();
        throw;
    } catch (const CmaException &ex) {
        return ex.code();
    } catch (const std::exception &e) {
        return CMA_LEDGER_ERROR_EXCEPTION;
    }
} catch (...) {
    try {
        get_last_err_msg_storage() = std::string("unknown error");
    } catch (...) {
        // Failed to allocate string, last resort is to set an empty error.
        get_last_err_msg_storage().clear();
    }
    return CMA_LEDGER_ERROR_UNKNOWN;
}

auto cma_ledger_result_success() -> int {
    get_last_err_msg_storage().clear();
    return CMA_LEDGER_SUCCESS;
}

} // namespace

/*
 * Ledger Api
 */

static_assert(sizeof(cma_ledger_t) >= sizeof(cma_ledger));
static_assert(alignof(cma_ledger_t) == alignof(cma_ledger));

auto cma_ledger_init(cma_ledger_t *ledger) -> int try {
    new (ledger) cma_ledger();
    return cma_ledger_result_success();
} catch (...) {
    return cma_ledger_result_failure();
}

auto cma_ledger_fini(cma_ledger_t *ledger) -> int try {
    if (ledger == nullptr) {
        throw CmaException("Invalid ledger", -EINVAL);
    }
    auto *ledger_ptr = reinterpret_cast<cma_ledger *>(ledger);
    if (!ledger_ptr->is_initialized()) {
        throw CmaException("Invalid ledger ptr", -EINVAL);
    }
    ledger_ptr->~cma_ledger();
    return cma_ledger_result_success();
} catch (...) {
    return cma_ledger_result_failure();
}

auto cma_ledger_reset(cma_ledger_t *ledger) -> int try {
    if (ledger == nullptr) {
        throw CmaException("Invalid ledger", -EINVAL);
    }
    auto *ledger_ptr = reinterpret_cast<cma_ledger *>(ledger);
    if (!ledger_ptr->is_initialized()) {
        throw CmaException("Invalid ledger ptr", -EINVAL);
    }

    ledger_ptr->clear();
    return cma_ledger_result_success();
} catch (...) {
    return cma_ledger_result_failure();
}

auto cma_ledger_retrieve_asset(cma_ledger_t *ledger, cma_ledger_asset_id_t *asset_id,
    cma_token_address_t *token_address, cma_token_id_t *token_id, cma_ledger_asset_type_t *asset_type,
    cma_ledger_retrieve_operation_t operation) -> int try {
    if (ledger == nullptr) {
        throw CmaException("Invalid ledger ptr", -EINVAL);
    }
    if (asset_type == nullptr) {
        throw CmaException("Invalid asset type ptr", -EINVAL);
    }
    auto *ledger_ptr = reinterpret_cast<cma_ledger *>(ledger);
    if (!ledger_ptr->is_initialized()) {
        throw CmaException("Invalid ledger ptr", -EINVAL);
    }
    ledger_ptr->retrieve_create_asset(asset_id, token_address, token_id, *asset_type, operation);
    return cma_ledger_result_success();
} catch (...) {
    return cma_ledger_result_failure();
}

auto cma_ledger_retrieve_account(cma_ledger_t *ledger, cma_ledger_account_id_t *account_id,
    cma_ledger_account_t *account, const void *addr_accid, cma_ledger_account_type_t *account_type,
    cma_ledger_retrieve_operation_t operation) -> int try {
    if (ledger == nullptr) {
        throw CmaException("Invalid ledger ptr", -EINVAL);
    }
    if (account_type == nullptr) {
        throw CmaException("Invalid account type ptr", -EINVAL);
    }

    auto *ledger_ptr = reinterpret_cast<cma_ledger *>(ledger);
    if (!ledger_ptr->is_initialized()) {
        throw CmaException("Invalid ledger ptr", -EINVAL);
    }

    ledger_ptr->retrieve_create_account(account_id, account, addr_accid, *account_type, operation);
    return cma_ledger_result_success();
} catch (...) {
    return cma_ledger_result_failure();
}

auto cma_ledger_get_total_supply(cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id, cma_amount_t *out_total_supply)
    -> int try {
    if (ledger == nullptr) {
        throw CmaException("Invalid ledger ptr", -EINVAL);
    }
    if (out_total_supply == nullptr) {
        throw CmaException("Invalid total supply ptr", -EINVAL);
    }
    auto *ledger_ptr = reinterpret_cast<cma_ledger *>(ledger);
    if (!ledger_ptr->is_initialized()) {
        throw CmaException("Invalid ledger ptr", -EINVAL);
    }
    if (!ledger_ptr->find_asset(asset_id, nullptr, nullptr, nullptr, out_total_supply)) {
        throw CmaException("Asset not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
    }

    return cma_ledger_result_success();
} catch (...) {
    return cma_ledger_result_failure();
}

auto cma_ledger_get_balance(cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t account_id,
    cma_amount_t *out_balance) -> int try {
    if (ledger == nullptr) {
        throw CmaException("Invalid ledger ptr", -EINVAL);
    }
    if (out_balance == nullptr) {
        throw CmaException("Invalid out balance ptr", -EINVAL);
    }
    auto *ledger_ptr = reinterpret_cast<cma_ledger *>(ledger);
    if (!ledger_ptr->is_initialized()) {
        throw CmaException("Invalid ledger ptr", -EINVAL);
    }
    ledger_ptr->get_account_asset_balance(asset_id, account_id, *out_balance);
    return cma_ledger_result_success();
} catch (...) {
    return cma_ledger_result_failure();
}

auto cma_ledger_deposit(cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t to_account_id,
    const cma_amount_t *deposit) -> int try {
    if (ledger == nullptr) {
        throw CmaException("Invalid ledger ptr", -EINVAL);
    }
    if (deposit == nullptr) {
        throw CmaException("Invalid deposit ptr", -EINVAL);
    }
    auto *ledger_ptr = reinterpret_cast<cma_ledger *>(ledger);
    if (!ledger_ptr->is_initialized()) {
        throw CmaException("Invalid ledger ptr", -EINVAL);
    }
    ledger_ptr->deposit(asset_id, to_account_id, *deposit);
    return cma_ledger_result_success();
} catch (...) {
    return cma_ledger_result_failure();
}

auto cma_ledger_withdraw(cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t from_account_id,
    const cma_amount_t *withdrawal) -> int try {
    if (ledger == nullptr) {
        throw CmaException("Invalid ledger ptr", -EINVAL);
    }
    if (withdrawal == nullptr) {
        throw CmaException("Invalid withdrawal ptr", -EINVAL);
    }
    auto *ledger_ptr = reinterpret_cast<cma_ledger *>(ledger);
    if (!ledger_ptr->is_initialized()) {
        throw CmaException("Invalid ledger ptr", -EINVAL);
    }
    ledger_ptr->withdraw(asset_id, from_account_id, *withdrawal);
    return cma_ledger_result_success();
} catch (...) {
    return cma_ledger_result_failure();
}

auto cma_ledger_transfer(cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t from_account_id,
    cma_ledger_account_id_t to_account_id, const cma_amount_t *amount) -> int try {
    if (ledger == nullptr) {
        throw CmaException("Invalid ledger ptr", -EINVAL);
    }
    if (amount == nullptr) {
        throw CmaException("Invalid amount ptr", -EINVAL);
    }
    auto *ledger_ptr = reinterpret_cast<cma_ledger *>(ledger);
    if (!ledger_ptr->is_initialized()) {
        throw CmaException("Invalid ledger ptr", -EINVAL);
    }
    ledger_ptr->transfer(asset_id, from_account_id, to_account_id, *amount);
    return cma_ledger_result_success();
} catch (...) {
    return cma_ledger_result_failure();
}

auto cma_ledger_get_last_error_message() -> const char * {
    return get_last_err_msg_storage().c_str();
}
