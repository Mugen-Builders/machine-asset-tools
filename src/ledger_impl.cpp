#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>
#include <tuple>
#include <utility>

extern "C" {
#include "libcma/ledger.h"
#include "libcma/types.h"
}

#include "ledger_impl.h"
#include "utils.h"

/*
 * Ledger class
 */

cma_ledger_base::~cma_ledger_base() {
    magic = 0;
}

auto cma_ledger_base::is_initialized() const -> bool {
    return magic == CMA_LEDGER_MAGIC;
}

void cma_ledger_basic::clear() {
    account_to_laccid.clear();
    laccid_to_account.clear();
    asset_to_lassid.clear();
    lassid_to_asset.clear();
    account_asset_balance.clear();
}

auto cma_ledger_basic::get_asset_count() -> size_t {
    return lassid_to_asset.size();
}

auto cma_ledger_basic::find_asset(cma_ledger_asset_id_t asset_id, cma_ledger_asset_type_t *asset_type,
    cma_token_address_t *token_address, cma_token_id_t *token_id, cma_amount_t *supply) -> bool {
    const auto find_result = lassid_to_asset.find(asset_id);
    if (find_result == lassid_to_asset.end()) {
        return false;
    }

    if (asset_type != nullptr) {
        *asset_type = find_result->second.type;
    }

    // asset found
    switch (find_result->second.type) {
        case CMA_LEDGER_ASSET_TYPE_ID:
            break;
        case CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS:
            if (token_address != nullptr) {
                std::ignore = std::copy_n(std::begin(find_result->second.token_address.data), CMA_ABI_ADDRESS_LENGTH,
                    std::begin(token_address->data));
            }
            break;
        case CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID:
            if (token_address != nullptr) {
                std::ignore = std::copy_n(std::begin(find_result->second.token_address.data), CMA_ABI_ADDRESS_LENGTH,
                    std::begin(token_address->data));
            }
            if (token_id != nullptr) {
                std::ignore = std::copy_n(std::begin(find_result->second.token_id.data), CMA_ABI_ID_LENGTH,
                    std::begin(token_id->data));
            }
            break;
        default:
            // shouldn't be here (wrongly added to map)
            throw CmaException("Invalid asset type", -EINVAL);
    }
    if (supply != nullptr) {
        std::ignore =
            std::copy_n(std::begin(find_result->second.supply.data), CMA_ABI_U256_LENGTH, std::begin(supply->data));
    }
    return true;
}

void cma_ledger_basic::set_asset_supply(cma_ledger_asset_id_t asset_id, cma_amount_t &supply) {
    auto find_result = lassid_to_asset.find(asset_id);
    if (find_result == lassid_to_asset.end()) {
        throw CmaException("Asset by id not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
    }

    std::ignore =
        std::copy_n(std::begin(supply.data), CMA_ABI_U256_LENGTH, std::begin(find_result->second.supply.data));
}

void cma_ledger_basic::retrieve_asset(cma_ledger_asset_id_t *asset_id, cma_token_address_t *token_address,
    cma_token_id_t *token_id, cma_amount_t *out_total_supply, cma_ledger_asset_type_t &asset_type,
    cma_ledger_retrieve_operation_t operation) {

    const size_t curr_size = get_asset_count();

    switch (asset_type) {
        case CMA_LEDGER_ASSET_TYPE_ID: {
            // find by id, create with no address and id (don't create asset to lassid)

            // 1: look for asset
            if (asset_id == nullptr) {
                throw CmaException("Invalid asset id ptr", -EINVAL);
            }
            if (operation == CMA_LEDGER_OP_FIND || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                if (find_asset(*asset_id, &asset_type, token_address, token_id, out_total_supply)) {
                    return;
                }
                if (operation == CMA_LEDGER_OP_FIND) {
                    throw CmaException("Asset not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
                }
            }

            // 2: create asset id map (but no reverse)
            if (operation == CMA_LEDGER_OP_CREATE || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                auto *new_asset = new cma_ledger_asset_struct_t();
                new_asset->type = CMA_LEDGER_ASSET_TYPE_ID;
                const std::pair<lassid_to_asset_t::iterator, bool> insertion_result =
                    lassid_to_asset.insert({curr_size, *new_asset});
                if (!insertion_result.second) {
                    // Key already existed, value was not overwritten
                    // shouldn't be here
                    throw CmaException("Asset ID already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }

                *asset_id = curr_size;
            }
            break;
        }
        case CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS: {
            // find by address, create with address (create asset to lassid)

            // 1: look for asset
            if (token_address == nullptr) {
                throw CmaException("Invalid token address ptr", -EINVAL);
            }

            cma_ledger_asset_key_bytes_t asset_key_bytes = {};
            std::span<uint8_t> asset_key_bytes_span(asset_key_bytes);
            std::span<uint8_t> asset_key_bytes_addr_span =
                asset_key_bytes_span.subspan(CMA_LEDGER_ASSET_ARRAY_KEY_ADDRESS_IND, CMA_ABI_ADDRESS_LENGTH);
            asset_key_bytes[CMA_LEDGER_ASSET_ARRAY_KEY_TYPE_IND] = CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS;
            std::ignore =
                std::copy_n(std::begin(token_address->data), CMA_ABI_ADDRESS_LENGTH, asset_key_bytes_addr_span.begin());
            cma_ledger_asset_key_t asset_key(asset_key_bytes_span.begin(), asset_key_bytes_span.end());

            if (operation == CMA_LEDGER_OP_FIND || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                auto find_result_addr = asset_to_lassid.find(asset_key);
                if (find_result_addr != asset_to_lassid.end()) {
                    if (!find_asset(find_result_addr->second, &asset_type, token_address, token_id, out_total_supply)) {
                        // shouldn't be here
                        throw CmaException("Asset by id not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
                    }
                    if (asset_id != nullptr) {
                        *asset_id = find_result_addr->second;
                    }
                    return;
                }
                if (operation == CMA_LEDGER_OP_FIND) {
                    throw CmaException("Asset not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
                }
            }

            // 2: create asset id map (and reverse token address, no token id)
            if (operation == CMA_LEDGER_OP_CREATE || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                std::pair<asset_to_lassid_t::iterator, bool> insertion_result_addr =
                    asset_to_lassid.insert({asset_key, curr_size});
                if (!insertion_result_addr.second) {
                    // Key already existed, value was not overwritten
                    throw CmaException("Asset Key already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }

                auto *new_asset = new cma_ledger_asset_struct_t();
                new_asset->type = CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS;
                std::ignore = std::copy_n(std::begin(token_address->data), CMA_ABI_ADDRESS_LENGTH,
                    std::begin(new_asset->token_address.data));
                std::pair<lassid_to_asset_t::iterator, bool> insertion_result =
                    lassid_to_asset.insert({curr_size, *new_asset});
                if (!insertion_result.second) {
                    // shouldn't be here
                    throw CmaException("Asset ID already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }

                if (asset_id != nullptr) {
                    *asset_id = curr_size;
                }
            }
            break;
        }
        case CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID: {
            // find by address and id, create with address and id (create asset to lassid)

            // 1: look for asset
            if (token_address == nullptr) {
                throw CmaException("Invalid token address ptr", -EINVAL);
            }
            if (token_id == nullptr) {
                throw CmaException("Invalid token id ptr", -EINVAL);
            }

            cma_ledger_asset_key_bytes_t asset_key_bytes = {};
            std::span<uint8_t> asset_key_bytes_span(asset_key_bytes);
            std::span<uint8_t> asset_key_bytes_addr_span =
                asset_key_bytes_span.subspan(CMA_LEDGER_ASSET_ARRAY_KEY_ADDRESS_IND, CMA_ABI_ADDRESS_LENGTH);
            std::span<uint8_t> asset_key_bytes_id_span =
                asset_key_bytes_span.subspan(CMA_LEDGER_ASSET_ARRAY_KEY_ID_IND, CMA_ABI_ID_LENGTH);
            asset_key_bytes[CMA_LEDGER_ASSET_ARRAY_KEY_TYPE_IND] = CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS;
            std::ignore =
                std::copy_n(std::begin(token_address->data), CMA_ABI_ADDRESS_LENGTH, asset_key_bytes_addr_span.begin());
            std::ignore = std::copy_n(std::begin(token_id->data), CMA_ABI_ID_LENGTH, asset_key_bytes_id_span.begin());
            cma_ledger_asset_key_t asset_key(asset_key_bytes_span.begin(), asset_key_bytes_span.end());

            if (operation == CMA_LEDGER_OP_FIND || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                auto find_result_addr = asset_to_lassid.find(asset_key);
                if (find_result_addr != asset_to_lassid.end()) {
                    if (!find_asset(find_result_addr->second, &asset_type, token_address, token_id, out_total_supply)) {
                        // shouldn't be here
                        throw CmaException("Asset by id not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
                    }
                    if (asset_id != nullptr) {
                        *asset_id = find_result_addr->second;
                    }
                    return;
                }
                if (operation == CMA_LEDGER_OP_FIND) {
                    throw CmaException("Asset not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
                }
            }

            // 2: create asset id map (and reverse token address, token id)
            if (operation == CMA_LEDGER_OP_CREATE || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                std::pair<asset_to_lassid_t::iterator, bool> insertion_result_addr =
                    asset_to_lassid.insert({asset_key, curr_size});
                if (!insertion_result_addr.second) {
                    // Key already existed, value was not overwritten
                    throw CmaException("Asset Key already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }

                auto *new_asset = new cma_ledger_asset_struct_t();
                new_asset->type = CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID;
                std::ignore = std::copy_n(std::begin(token_address->data), CMA_ABI_ADDRESS_LENGTH,
                    std::begin(new_asset->token_address.data));
                std::ignore =
                    std::copy_n(std::begin(token_id->data), CMA_ABI_ID_LENGTH, std::begin(new_asset->token_id.data));
                std::pair<lassid_to_asset_t::iterator, bool> insertion_result =
                    lassid_to_asset.insert({curr_size, *new_asset});
                if (!insertion_result.second) {
                    // shouldn't be here
                    throw CmaException("Asset ID already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }

                if (asset_id != nullptr) {
                    *asset_id = curr_size;
                }
            }
            break;
        }
        default:
            throw CmaException("Invalid asset type", -EINVAL);
    }
}

auto cma_ledger_basic::get_account_count() -> size_t {
    return laccid_to_account.size();
}

auto cma_ledger_basic::find_account(cma_ledger_account_id_t account_id, cma_ledger_account_t *account, size_t *)
    -> bool {
    auto find_result = laccid_to_account.find(account_id);
    if (find_result == laccid_to_account.end()) {
        return false;
    }

    // asset found
    if (account != nullptr) {
        account->type = find_result->second.type;
        std::ignore = std::copy_n(std::begin(find_result->second.account_id.data), CMA_ABI_ID_LENGTH,
            std::begin(account->account_id.data));
    }
    return true;
}

void cma_ledger_basic::retrieve_account(cma_ledger_account_id_t *account_id, cma_ledger_account_t *account,
    const void *addr_accid, size_t *, cma_ledger_account_type_t &account_type,
    cma_ledger_retrieve_operation_t operation) {
    const size_t curr_size = get_account_count();

    switch (account_type) {
        case CMA_LEDGER_ACCOUNT_TYPE_ID: {
            // find by id, create with no account (don't create account to laccid)
            // ignores type in account struct

            // 1: look for account_id
            if (account_id == nullptr) {
                throw CmaException("Invalid account id ptr", -EINVAL);
            }
            if (operation == CMA_LEDGER_OP_FIND || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                if (cma_ledger_basic::find_account(*account_id, account, nullptr)) {
                    if (account != nullptr) {
                        account_type = account->type;
                    }
                    return;
                }
                if (operation == CMA_LEDGER_OP_FIND) {
                    throw CmaException("Account not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
                }
            }

            // 2: create account id map (but no reverse)
            if (operation == CMA_LEDGER_OP_CREATE || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                auto *new_account = new cma_ledger_account_t();
                new_account->type = CMA_LEDGER_ACCOUNT_TYPE_ID;
                std::pair<laccid_to_account_t::iterator, bool> insertion_result =
                    laccid_to_account.insert({curr_size, *new_account});
                if (!insertion_result.second) {
                    // shouldn't be here
                    throw CmaException("Account ID already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }

                *account_id = curr_size;
            }
            break;
        }
        case CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS:
        case CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID: {
            // find by wallet address, create with address (create account to laccid)

            cma_ledger_account_t account_local;
            if (addr_accid != nullptr) {
                if (account_type == CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS) {
                    std::ignore = std::copy_n((static_cast<const cma_abi_address_t *>(addr_accid))->data,
                        CMA_ABI_ADDRESS_LENGTH, static_cast<uint8_t *>(account_local.address.data));
                } else if (account_type == CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID) {
                    std::ignore = std::copy_n((static_cast<const cma_account_id_t *>(addr_accid))->data,
                        CMA_ABI_ID_LENGTH, static_cast<uint8_t *>(account_local.account_id.data));
                }
            } else {
                if (account == nullptr) {
                    throw CmaException("Invalid account ptr", -EINVAL);
                }
                std::ignore = std::copy_n(std::begin(account->account_id.data), CMA_ABI_ID_LENGTH,
                    std::begin(account_local.account_id.data));
            }

            // 1: look for account
            // when using address ensure that fix bytes are zeroed
            if (account_type == CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS) {
                memset(account_local.fix, 0, CMA_ABI_ID_LENGTH - CMA_ABI_ADDRESS_LENGTH);
            }
            cma_ledger_account_key_t account_key(reinterpret_cast<const char *>(account_local.account_id.data),
                CMA_ABI_ID_LENGTH);

            if (operation == CMA_LEDGER_OP_FIND || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                auto find_result_acc = account_to_laccid.find(account_key);
                if (find_result_acc != account_to_laccid.end()) {
                    if (!cma_ledger_basic::find_account(find_result_acc->second, &account_local, nullptr)) {
                        // shouldn't be here
                        throw CmaException("Account by id not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
                    }
                    if (account_id != nullptr) {
                        *account_id = find_result_acc->second;
                    }
                    if (account != nullptr) {
                        account->type = account_local.type;
                        account_type = account_local.type;
                        std::ignore = std::copy_n(std::begin(account_local.account_id.data), CMA_ABI_ID_LENGTH,
                            std::begin(account->account_id.data));
                    }
                    return;
                }
                if (operation == CMA_LEDGER_OP_FIND) {
                    throw CmaException("Account not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
                }
            }

            // 2: create account id map (and reverse account)
            if (operation == CMA_LEDGER_OP_CREATE || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                std::pair<account_to_laccid_t::iterator, bool> insertion_result_acc =
                    account_to_laccid.insert({account_key, curr_size});
                if (!insertion_result_acc.second) {
                    // Key already existed, value was not overwritten
                    throw CmaException("Account Key already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }

                account_local.type = account_type; // set correct type
                std::pair<laccid_to_account_t::iterator, bool> insertion_result =
                    laccid_to_account.insert({curr_size, account_local});
                if (!insertion_result.second) {
                    // shouldn't be here
                    throw CmaException("Account ID already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }

                if (account_id != nullptr) {
                    *account_id = curr_size;
                }
                if (account != nullptr) {
                    account->type = account_local.type;
                    std::ignore = std::copy_n(std::begin(account_local.account_id.data), CMA_ABI_ID_LENGTH,
                        std::begin(account->account_id.data));
                }
            }
            break;
        }
        default:
            throw CmaException("Invalid asset type", -EINVAL);
    }
}

void cma_ledger_basic::get_account_asset_balance(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t account_id,
    cma_amount_t *balance, cma_ledger_account_balance_info_t *account_balance_info) {
    auto find_result = account_asset_balance.find({asset_id, account_id});

    if (account_balance_info != nullptr) {
        throw CmaException("Account balance not available", -CMA_LEDGER_ERROR_BALANCE_NOT_FOUND);
    }
    if (balance != nullptr) {
        if (find_result == account_asset_balance.end()) {
            std::fill_n(std::begin(balance->data), CMA_ABI_U256_LENGTH, (uint8_t) 0);
            return;
        }
        std::ignore = std::copy_n(std::begin(find_result->second.data), CMA_ABI_U256_LENGTH, std::begin(balance->data));
    }
}

void cma_ledger_basic::set_account_asset_balance(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t account_id,
    const cma_amount_t &balance) {
    auto find_result = account_asset_balance.find({asset_id, account_id});
    if (find_result == account_asset_balance.end()) {
        // create new entry
        account_asset_balance.insert({{asset_id, account_id}, balance});
        return;
    }

    std::ignore = std::copy_n(balance.data, CMA_ABI_U256_LENGTH, std::begin(find_result->second.data));
}

void cma_ledger_basic::deposit(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t to_account_id,
    const cma_amount_t &deposit) {
    // 1: check asset
    cma_amount_t curr_supply = {};
    if (!find_asset(asset_id, nullptr, nullptr, nullptr, &curr_supply)) {
        throw CmaException("Asset by id not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
    }

    cma_amount_t new_supply = {};
    if (!amount_checked_add(new_supply, curr_supply, deposit)) {
        throw CmaException("Asset supply overflow", CMA_LEDGER_ERROR_SUPPLY_OVERFLOW);
    }
    // 2: check account
    if (!cma_ledger_basic::find_account(to_account_id, nullptr, nullptr)) {
        throw CmaException("Account by id not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
    }
    // 3: check amount
    cma_amount_t curr_balance = {};
    get_account_asset_balance(asset_id, to_account_id, &curr_balance, nullptr);

    cma_amount_t new_balance = {};
    if (!amount_checked_add(new_balance, curr_balance, deposit)) {
        throw CmaException("Asset balance overflow", CMA_LEDGER_ERROR_BALANCE_OVERFLOW);
    }

    // 4: update account balance
    set_account_asset_balance(asset_id, to_account_id, new_balance);

    // 5: update asset supply
    set_asset_supply(asset_id, new_supply);
}

void cma_ledger_basic::withdraw(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t from_account_id,
    const cma_amount_t &withdrawal) {
    // 1: check asset
    cma_amount_t curr_supply = {};
    if (!find_asset(asset_id, nullptr, nullptr, nullptr, &curr_supply)) {
        throw CmaException("Asset by id not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
    }

    cma_amount_t new_supply = {};
    if (!amount_checked_sub(new_supply, curr_supply, withdrawal)) {
        throw CmaException("Asset supply underflow", CMA_LEDGER_ERROR_SUPPLY_OVERFLOW);
    }
    // 2: check account
    if (!cma_ledger_basic::find_account(from_account_id, nullptr, nullptr)) {
        throw CmaException("Account by id not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
    }
    // 3: check amount
    cma_amount_t curr_balance = {};
    get_account_asset_balance(asset_id, from_account_id, &curr_balance, nullptr);

    cma_amount_t new_balance = {};
    if (!amount_checked_sub(new_balance, curr_balance, withdrawal)) {
        throw CmaException("Insufficient funds", CMA_LEDGER_ERROR_INSUFFICIENT_FUNDS);
    }

    // 4: update account balance
    set_account_asset_balance(asset_id, from_account_id, new_balance);

    // 5: update asset supply
    set_asset_supply(asset_id, new_supply);
}

void cma_ledger_basic::transfer(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t from_account_id,
    cma_ledger_account_id_t to_account_id, const cma_amount_t &amount) {
    // 1: check asset
    cma_amount_t curr_supply = {};
    if (!find_asset(asset_id, nullptr, nullptr, nullptr, &curr_supply)) {
        throw CmaException("Asset by id not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
    }

    // 2: check account from
    if (from_account_id == to_account_id) {
        throw CmaException("Account from equal to account to", -EINVAL);
    }
    if (!cma_ledger_basic::find_account(from_account_id, nullptr, nullptr)) {
        throw CmaException("Account from not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
    }
    // 3: check amount from
    cma_amount_t curr_balance_from = {};
    get_account_asset_balance(asset_id, from_account_id, &curr_balance_from, nullptr);

    cma_amount_t new_balance_from = {};
    if (!amount_checked_sub(new_balance_from, curr_balance_from, amount)) {
        throw CmaException("Insufficient funds", CMA_LEDGER_ERROR_INSUFFICIENT_FUNDS);
    }

    // 4: check account to
    if (!cma_ledger_basic::find_account(to_account_id, nullptr, nullptr)) {
        throw CmaException("Account to not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
    }
    // 5: check amount to
    cma_amount_t curr_balance_to = {};
    get_account_asset_balance(asset_id, to_account_id, &curr_balance_to, nullptr);

    cma_amount_t new_balance_to = {};
    if (!amount_checked_add(new_balance_to, curr_balance_to, amount)) {
        throw CmaException("Balance overflow", CMA_LEDGER_ERROR_BALANCE_OVERFLOW);
    }

    // 6: update account balances
    set_account_asset_balance(asset_id, from_account_id, new_balance_from);
    set_account_asset_balance(asset_id, to_account_id, new_balance_to);
}

/*
 * Ledger File Impl
 */

static constexpr size_t INIT_ACCOUNTS_CAPACITY = 256; //< Initial capacity for accounts map.
static constexpr size_t INIT_ASSETS_CAPACITY = 32;    //< Initial capacity for assets map.
static constexpr size_t HINT_ASSETS_PER_ACCOUNT = 8;  //< Average of positions for an account.
static constexpr size_t INIT_BALANCE = HINT_ASSETS_PER_ACCOUNT * INIT_ACCOUNTS_CAPACITY;
// static constexpr size_t MAX_ACCOUNTS = 16UL * 1024;           //< Maximum number of accounts.
// static constexpr size_t MAX_ASSETS = 256UL;             //< Maximum number of assets.
// static constexpr size_t MAX_MEMORY_SIZE = 32UL * 1024 * 1024; //< Ledger state maximum memory usage.

size_t cma_ledger_memory::estimate_required_size(size_t n_accounts, size_t n_assets, size_t n_balances) {
    // printf("estimate_required_size - n_accounts: %zu - n_assets: %zu - n_balances: %zu\n",
    //     n_accounts, n_assets, n_balances);
    // printf("  void_allocator: %zu\n", sizeof(interprocess::void_allocator));
    // printf("  lassid_to_asset_t: %zu\n", sizeof(lassid_to_asset_t));
    // printf("  asset_to_lassid_t: %zu\n", sizeof(asset_to_lassid_t));
    // printf("  cma_ledger_asset_id_t: %zu (%zu * 2)\n", sizeof(cma_ledger_asset_id_t),
    // n_assets*sizeof(cma_ledger_asset_id_t)); printf("  cma_ledger_asset_struct_t: %zu (%zu)\n",
    // sizeof(cma_ledger_asset_struct_t), n_assets*sizeof(cma_ledger_asset_struct_t)); printf("
    // cma_ledger_asset_key_bytes_t: %zu (%zu)\n", sizeof(cma_ledger_asset_key_bytes_t),
    // n_assets*sizeof(cma_ledger_asset_key_bytes_t)); printf("  laccid_to_account_t: %zu\n",
    // sizeof(laccid_to_account_t)); printf("  account_to_laccid_t: %zu\n", sizeof(account_to_laccid_t)); printf("
    // cma_ledger_account_id_t: %zu (%zu * 2)\n", sizeof(cma_ledger_account_id_t),
    // n_accounts*sizeof(cma_ledger_account_id_t)); printf("  cma_ledger_account_struct_t: %zu (%zu)\n",
    // sizeof(cma_ledger_account_struct_t), n_accounts*sizeof(cma_ledger_account_struct_t)); printf("
    // cma_ledger_account_key_bytes_t: %zu (%zu)\n", sizeof(cma_ledger_account_key_bytes_t),
    // n_accounts*sizeof(cma_ledger_account_key_bytes_t));

    // printf("  account_asset_map_t: %zu\n", sizeof(account_asset_map_t));
    // printf("  cma_map_key_t: %zu (%zu)\n", sizeof(cma_map_key_t), n_balances*sizeof(cma_map_key_t));
    // printf("  cma_balance_t: %zu (%zu)\n", sizeof(cma_balance_t), n_balances*sizeof(cma_balance_t));
    // printf("  cma_ledger_account_balance_t: %zu (%zu)\n", sizeof(cma_ledger_account_balance_t),
    // n_balances*sizeof(cma_ledger_account_balance_t)); printf("  cma_map_key_t: %zu (%zu * 2)\n",
    // sizeof(cma_map_key_t), n_balances*sizeof(cma_map_key_t));

    return (sizeof(interprocess::void_allocator) +
               (sizeof(lassid_to_asset_t) +
                   n_assets * (sizeof(cma_ledger_asset_id_t) + sizeof(cma_ledger_asset_struct_t))) +
               (sizeof(asset_to_lassid_t) +
                   n_assets * (sizeof(cma_ledger_asset_key_bytes_t) + sizeof(cma_ledger_asset_id_t))) +
               (sizeof(laccid_to_account_t) +
                   n_accounts * (sizeof(cma_ledger_account_id_t) + sizeof(cma_ledger_account_struct_t))) +
               (sizeof(account_to_laccid_t) +
                   n_accounts * (sizeof(cma_ledger_account_key_bytes_t) + sizeof(cma_ledger_account_id_t))) +
               (sizeof(account_asset_map_t) +
                   n_balances *
                       (sizeof(cma_map_key_t) + sizeof(cma_balance_t) +
                           sizeof(cma_ledger_account_balance_t) +        // sizeof(cma_ledger_account_virtual_balance_t)
                           sizeof(cma_map_key_t) + sizeof(cma_map_key_t) // last keys lists
                           )) +
               3 * sizeof(cma_ledger_asset_id_t) + sizeof(bool)) *
        5 / 4 // security factor
        + CMA_LEDGER_MIN_MEM_LENGTH;
}

cma_ledger_memory::cma_ledger_memory(interprocess::open_only_t mode, const char *memory_file_name, size_t offset,
    size_t mem_length, size_t n_accounts, size_t n_assets, size_t n_balances) :
    max_accounts(n_accounts),
    max_assets(n_assets),
    max_balances(n_balances),
    m_file(memory_file_name, interprocess::read_write),
    m_region(m_file, interprocess::read_write, offset, mem_length),
    m_memory(mode, m_region.get_address(), m_region.get_size()),
    m_allocator(*m_memory.find_or_construct<interprocess::void_allocator>(interprocess::unique_instance)(
        m_memory.get_segment_manager())),
    balances{
        *m_memory.find_or_construct<balance_list_t>(interprocess::unique_instance)(0, m_memory.get_segment_manager())},
    virtual_balances{*m_memory.find_or_construct<virtual_balance_list_t>(
        interprocess::unique_instance)(0, m_memory.get_segment_manager())},
    lassid_to_asset{*m_memory.find_or_construct<lassid_to_asset_t>(
        interprocess::unique_instance)(INIT_ASSETS_CAPACITY, m_memory.get_segment_manager())},
    asset_to_lassid{*m_memory.find_or_construct<asset_to_lassid_t>(
        interprocess::unique_instance)(INIT_ASSETS_CAPACITY, m_memory.get_segment_manager())},
    laccid_to_account{*m_memory.find_or_construct<laccid_to_account_t>(
        interprocess::unique_instance)(INIT_ACCOUNTS_CAPACITY, m_memory.get_segment_manager())},
    account_to_laccid{*m_memory.find_or_construct<account_to_laccid_t>(
        interprocess::unique_instance)(INIT_ACCOUNTS_CAPACITY, m_memory.get_segment_manager())},
    account_asset_balance{*m_memory.find_or_construct<account_asset_map_t>(
        interprocess::unique_instance)(INIT_BALANCE, m_memory.get_segment_manager())},
    last_balances{*m_memory.find_or_construct<balance_key_list_t>("last_balances")(0, m_memory.get_segment_manager())},
    last_virtual_balances{
        *m_memory.find_or_construct<balance_key_list_t>("last_virtual_balances")(0, m_memory.get_segment_manager())},
    next_asset_id{*m_memory.find_or_construct<cma_ledger_asset_id_t>("next_asset_id")(0)},
    next_account_id{*m_memory.find_or_construct<cma_ledger_account_id_t>("next_account_id")(0)},
    base_asset_id{*m_memory.find_or_construct<cma_ledger_asset_id_t>("base_asset_id")(0)},
    base_asset_id_defined{*m_memory.find_or_construct<bool>("base_asset_id_defined")(false)} {
    size_t required_size = cma_ledger_memory::estimate_required_size(max_accounts, max_assets, max_balances);
    if (required_size > m_region.get_size()) {
        throw CmaException("Mem length too small", -ENOBUFS);
    }
    balances.reserve(n_balances);
    virtual_balances.reserve(INIT_BALANCE);
    last_balances.reserve(INIT_BALANCE);
    last_virtual_balances.reserve(INIT_BALANCE);
}

cma_ledger_memory::cma_ledger_memory(interprocess::create_only_t mode, const char *memory_file_name, size_t offset,
    size_t mem_length, size_t n_accounts, size_t n_assets, size_t n_balances) :
    max_accounts(n_accounts),
    max_assets(n_assets),
    max_balances(n_balances),
    m_file(memory_file_name, interprocess::read_write),
    m_region(m_file, interprocess::read_write, offset, mem_length),
    m_memory(mode, m_region.get_address(), m_region.get_size()),
    m_allocator(*m_memory.find_or_construct<interprocess::void_allocator>(interprocess::unique_instance)(
        m_memory.get_segment_manager())),
    balances{
        *m_memory.find_or_construct<balance_list_t>(interprocess::unique_instance)(0, m_memory.get_segment_manager())},
    virtual_balances{*m_memory.find_or_construct<virtual_balance_list_t>(
        interprocess::unique_instance)(0, m_memory.get_segment_manager())},
    lassid_to_asset{*m_memory.find_or_construct<lassid_to_asset_t>(
        interprocess::unique_instance)(INIT_ASSETS_CAPACITY, m_memory.get_segment_manager())},
    asset_to_lassid{*m_memory.find_or_construct<asset_to_lassid_t>(
        interprocess::unique_instance)(INIT_ASSETS_CAPACITY, m_memory.get_segment_manager())},
    laccid_to_account{*m_memory.find_or_construct<laccid_to_account_t>(
        interprocess::unique_instance)(INIT_ACCOUNTS_CAPACITY, m_memory.get_segment_manager())},
    account_to_laccid{*m_memory.find_or_construct<account_to_laccid_t>(
        interprocess::unique_instance)(INIT_ACCOUNTS_CAPACITY, m_memory.get_segment_manager())},
    account_asset_balance{*m_memory.find_or_construct<account_asset_map_t>(
        interprocess::unique_instance)(INIT_BALANCE, m_memory.get_segment_manager())},
    last_balances{*m_memory.find_or_construct<balance_key_list_t>("last_balances")(0, m_memory.get_segment_manager())},
    last_virtual_balances{
        *m_memory.find_or_construct<balance_key_list_t>("last_virtual_balances")(0, m_memory.get_segment_manager())},
    next_asset_id{*m_memory.find_or_construct<cma_ledger_asset_id_t>("next_asset_id")(0)},
    next_account_id{*m_memory.find_or_construct<cma_ledger_account_id_t>("next_account_id")(0)},
    base_asset_id{*m_memory.find_or_construct<cma_ledger_asset_id_t>("base_asset_id")(0)},
    base_asset_id_defined{*m_memory.find_or_construct<bool>("base_asset_id_defined")(false)} {
    size_t required_size = cma_ledger_memory::estimate_required_size(max_accounts, max_assets, max_balances);
    if (required_size > m_region.get_size()) {
        throw CmaException("Mem length too small", -ENOBUFS);
    }
    balances.reserve(n_balances);
    virtual_balances.reserve(INIT_BALANCE);
    last_balances.reserve(INIT_BALANCE);
    last_virtual_balances.reserve(INIT_BALANCE);
}

cma_ledger_memory::cma_ledger_memory(void *mem_ptr, size_t mem_length, size_t n_accounts, size_t n_assets,
    size_t n_balances) :
    max_accounts(n_accounts),
    max_assets(n_assets),
    max_balances(n_balances),
    m_file(),
    m_region(),
    m_memory(interprocess::create_only, mem_ptr, mem_length),
    m_allocator(*m_memory.find_or_construct<interprocess::void_allocator>(interprocess::unique_instance)(
        m_memory.get_segment_manager())),
    balances{
        *m_memory.find_or_construct<balance_list_t>(interprocess::unique_instance)(0, m_memory.get_segment_manager())},
    virtual_balances{*m_memory.find_or_construct<virtual_balance_list_t>(
        interprocess::unique_instance)(0, m_memory.get_segment_manager())},
    lassid_to_asset{*m_memory.find_or_construct<lassid_to_asset_t>(
        interprocess::unique_instance)(INIT_ASSETS_CAPACITY, m_memory.get_segment_manager())},
    asset_to_lassid{*m_memory.find_or_construct<asset_to_lassid_t>(
        interprocess::unique_instance)(INIT_ASSETS_CAPACITY, m_memory.get_segment_manager())},
    laccid_to_account{*m_memory.find_or_construct<laccid_to_account_t>(
        interprocess::unique_instance)(INIT_ACCOUNTS_CAPACITY, m_memory.get_segment_manager())},
    account_to_laccid{*m_memory.find_or_construct<account_to_laccid_t>(
        interprocess::unique_instance)(INIT_ACCOUNTS_CAPACITY, m_memory.get_segment_manager())},
    account_asset_balance{*m_memory.find_or_construct<account_asset_map_t>(
        interprocess::unique_instance)(INIT_BALANCE, m_memory.get_segment_manager())},
    last_balances{*m_memory.find_or_construct<balance_key_list_t>("last_balances")(0, m_memory.get_segment_manager())},
    last_virtual_balances{
        *m_memory.find_or_construct<balance_key_list_t>("last_virtual_balances")(0, m_memory.get_segment_manager())},
    next_asset_id{*m_memory.find_or_construct<cma_ledger_asset_id_t>("next_asset_id")(0)},
    next_account_id{*m_memory.find_or_construct<cma_ledger_account_id_t>("next_account_id")(0)},
    base_asset_id{*m_memory.find_or_construct<cma_ledger_asset_id_t>("base_asset_id")(0)},
    base_asset_id_defined{*m_memory.find_or_construct<bool>("base_asset_id_defined")(false)} {
    size_t required_size = cma_ledger_memory::estimate_required_size(max_accounts, max_assets, max_balances);
    if (required_size > mem_length) {
        throw CmaException("Mem length too small", -ENOBUFS);
    }
    balances.reserve(n_balances);
    virtual_balances.reserve(INIT_BALANCE);
    last_balances.reserve(INIT_BALANCE);
    last_virtual_balances.reserve(INIT_BALANCE);
}

void cma_ledger_memory::clear() {
    balances.clear();
    virtual_balances.clear();
    account_to_laccid.clear();
    laccid_to_account.clear();
    asset_to_lassid.clear();
    lassid_to_asset.clear();
    account_asset_balance.clear();
    if (m_region.get_address() != nullptr) {
        m_region.flush();
    }
}

auto cma_ledger_memory::get_asset_count() -> size_t {
    return lassid_to_asset.size();
}

auto cma_ledger_memory::find_asset(cma_ledger_asset_id_t asset_id, cma_ledger_asset_type_t *asset_type,
    cma_token_address_t *token_address, cma_token_id_t *token_id, cma_amount_t *supply) -> bool {
    const auto find_result = lassid_to_asset.find(asset_id);
    if (find_result == lassid_to_asset.end()) {
        return false;
    }

    if (asset_type != nullptr) {
        *asset_type = find_result->second.type;
    }

    // asset found
    switch (find_result->second.type) {
        case CMA_LEDGER_ASSET_TYPE_ID:
        case CMA_LEDGER_ASSET_TYPE_BASE:
            break;
        case CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS:
            if (token_address != nullptr) {
                std::ignore = std::copy_n(std::begin(find_result->second.token_address.data), CMA_ABI_ADDRESS_LENGTH,
                    std::begin(token_address->data));
            }
            break;
        case CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID:
            if (token_address != nullptr) {
                std::ignore = std::copy_n(std::begin(find_result->second.token_address.data), CMA_ABI_ADDRESS_LENGTH,
                    std::begin(token_address->data));
            }
            if (token_id != nullptr) {
                std::ignore = std::copy_n(std::begin(find_result->second.token_id.data), CMA_ABI_ID_LENGTH,
                    std::begin(token_id->data));
            }
            break;
        default:
            // shouldn't be here (wrongly added to map)
            throw CmaException("Invalid asset type", -EINVAL);
    }
    if (supply != nullptr) {
        std::ignore =
            std::copy_n(std::begin(find_result->second.supply.data), CMA_ABI_U256_LENGTH, std::begin(supply->data));
    }
    return true;
}

auto cma_ledger_memory::remove_asset(cma_ledger_asset_id_t asset_id) -> void {
    const auto find_result = lassid_to_asset.find(asset_id);
    if (find_result == lassid_to_asset.end()) {
        throw CmaException("Couldn't find asset to remove", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
    }
    if (!is_zero(find_result->second.supply)) {
        throw CmaException("Asset still have supply", CMA_LEDGER_ERROR_ASSET_SUPPLY);
    }
    switch (find_result->second.type) {
        case CMA_LEDGER_ASSET_TYPE_ID: {
            break;
        }
        case CMA_LEDGER_ASSET_TYPE_BASE: {
            base_asset_id_defined = false;
            break;
        }
        case CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS: {
            cma_ledger_asset_key_bytes_t asset_key = {0};
            std::span<uint8_t> asset_key_bytes_span(asset_key);
            std::span<uint8_t> asset_key_bytes_addr_span =
                asset_key_bytes_span.subspan(CMA_LEDGER_ASSET_ARRAY_KEY_ADDRESS_IND, CMA_ABI_ADDRESS_LENGTH);
            asset_key[CMA_LEDGER_ASSET_ARRAY_KEY_TYPE_IND] = find_result->second.type;
            std::ignore = std::copy_n(std::begin(find_result->second.token_address.data), CMA_ABI_ADDRESS_LENGTH,
                asset_key_bytes_addr_span.begin());
            if (asset_to_lassid.erase(asset_key) == 0) {
                throw CmaException("Coundn't erase asset key map", CMA_LEDGER_ERROR_REMOVE);
            }
            break;
        }
        case CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID: {
            cma_ledger_asset_key_bytes_t asset_key = {0};
            std::span<uint8_t> asset_key_bytes_span(asset_key);
            std::span<uint8_t> asset_key_bytes_addr_span =
                asset_key_bytes_span.subspan(CMA_LEDGER_ASSET_ARRAY_KEY_ADDRESS_IND, CMA_ABI_ADDRESS_LENGTH);
            std::span<uint8_t> asset_key_bytes_id_span =
                asset_key_bytes_span.subspan(CMA_LEDGER_ASSET_ARRAY_KEY_ID_IND, CMA_ABI_ID_LENGTH);
            asset_key[CMA_LEDGER_ASSET_ARRAY_KEY_TYPE_IND] = find_result->second.type;
            std::ignore = std::copy_n(std::begin(find_result->second.token_address.data), CMA_ABI_ADDRESS_LENGTH,
                asset_key_bytes_addr_span.begin());
            std::ignore = std::copy_n(std::begin(find_result->second.token_id.data), CMA_ABI_ID_LENGTH,
                asset_key_bytes_id_span.begin());
            if (asset_to_lassid.erase(asset_key) == 0) {
                throw CmaException("Coundn't erase asset key map", CMA_LEDGER_ERROR_REMOVE);
            }
            break;
        }
        default: {
            // shouldn't be here (wrongly added to map)
            throw CmaException("Invalid asset type", -EINVAL);
        }
    }

    if (lassid_to_asset.erase(asset_id) == 0) {
        throw CmaException("Coundn't erase asset id map", CMA_LEDGER_ERROR_REMOVE);
    }
}

void cma_ledger_memory::set_asset_supply(cma_ledger_asset_id_t asset_id, cma_amount_t &supply) {
    auto find_result = lassid_to_asset.find(asset_id);
    if (find_result == lassid_to_asset.end()) {
        throw CmaException("Asset by id not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
    }

    std::ignore =
        std::copy_n(std::begin(supply.data), CMA_ABI_U256_LENGTH, std::begin(find_result->second.supply.data));
}

void cma_ledger_memory::retrieve_asset(cma_ledger_asset_id_t *asset_id, cma_token_address_t *token_address,
    cma_token_id_t *token_id, cma_amount_t *out_total_supply, cma_ledger_asset_type_t &asset_type,
    cma_ledger_retrieve_operation_t operation) {

    switch (asset_type) {
        case CMA_LEDGER_ASSET_TYPE_ID:
        case CMA_LEDGER_ASSET_TYPE_BASE: {
            // find by id, create with no address and id (don't create asset to lassid)

            // 1: look for asset
            if (asset_id == nullptr) {
                throw CmaException("Invalid asset id ptr", -EINVAL);
            }
            if (operation == CMA_LEDGER_OP_FIND || operation == CMA_LEDGER_OP_FIND_OR_CREATE ||
                operation == CMA_LEDGER_OP_FIND_AND_REMOVE) {

                if (operation == CMA_LEDGER_OP_FIND && asset_type == CMA_LEDGER_ASSET_TYPE_BASE && base_asset_id_defined) {
                    *asset_id = base_asset_id;
                }
                if (find_asset(*asset_id, &asset_type, token_address, token_id, out_total_supply)) {
                    if (operation == CMA_LEDGER_OP_FIND_AND_REMOVE) {
                        remove_asset(*asset_id);
                    }
                    return;
                }
                if (operation == CMA_LEDGER_OP_FIND || operation == CMA_LEDGER_OP_FIND_AND_REMOVE) {
                    throw CmaException("Asset not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
                }
            }

            // 2: create asset id map (but no reverse)
            if (operation == CMA_LEDGER_OP_CREATE || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                if (next_asset_id >= max_assets) {
                    throw CmaException("Max assets reached", CMA_LEDGER_ERROR_MAX_ASSETS_REACHED);
                }
                if (asset_type == CMA_LEDGER_ASSET_TYPE_BASE && base_asset_id_defined) {
                    throw CmaException("Base asset already inserted", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }
                auto *new_asset = new cma_ledger_asset_struct_t();
                new_asset->type = asset_type;
                const std::pair<lassid_to_asset_t::iterator, bool> insertion_result =
                    lassid_to_asset.insert({next_asset_id, *new_asset});
                if (!insertion_result.second) {
                    // Key already existed, value was not overwritten
                    // shouldn't be here
                    throw CmaException("Asset ID already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }

                *asset_id = next_asset_id;
                if (asset_type == CMA_LEDGER_ASSET_TYPE_BASE && !base_asset_id_defined) {
                    base_asset_id = *asset_id;
                    base_asset_id_defined = true;
                }
                next_asset_id++;
            }
            break;
        }
        case CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS: {
            // find by address, create with address (create asset to lassid)

            // 1: look for asset
            if (token_address == nullptr) {
                throw CmaException("Invalid token address ptr", -EINVAL);
            }

            cma_ledger_asset_key_bytes_t asset_key = {};
            std::span<uint8_t> asset_key_bytes_span(asset_key);
            std::span<uint8_t> asset_key_bytes_addr_span =
                asset_key_bytes_span.subspan(CMA_LEDGER_ASSET_ARRAY_KEY_ADDRESS_IND, CMA_ABI_ADDRESS_LENGTH);
            asset_key[CMA_LEDGER_ASSET_ARRAY_KEY_TYPE_IND] = asset_type;
            std::ignore =
                std::copy_n(std::begin(token_address->data), CMA_ABI_ADDRESS_LENGTH, asset_key_bytes_addr_span.begin());

            if (operation == CMA_LEDGER_OP_FIND || operation == CMA_LEDGER_OP_FIND_OR_CREATE ||
                operation == CMA_LEDGER_OP_FIND_AND_REMOVE) {
                auto find_result_addr = asset_to_lassid.find(asset_key);
                if (find_result_addr != asset_to_lassid.end()) {
                    if (!find_asset(find_result_addr->second, &asset_type, token_address, token_id, out_total_supply)) {
                        // shouldn't be here
                        throw CmaException("Asset by id not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
                    }
                    if (asset_id != nullptr) {
                        *asset_id = find_result_addr->second;
                    }
                    if (operation == CMA_LEDGER_OP_FIND_AND_REMOVE) {
                        remove_asset(find_result_addr->second);
                    }
                    return;
                }
                if (operation == CMA_LEDGER_OP_FIND || operation == CMA_LEDGER_OP_FIND_AND_REMOVE) {
                    throw CmaException("Asset not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
                }
            }

            // 2: create asset id map (and reverse token address, no token id)
            if (operation == CMA_LEDGER_OP_CREATE || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                if (next_asset_id >= max_assets) {
                    throw CmaException("Max assets reached", CMA_LEDGER_ERROR_MAX_ASSETS_REACHED);
                }
                std::pair<asset_to_lassid_t::iterator, bool> insertion_result_addr =
                    asset_to_lassid.insert({asset_key, next_asset_id});
                if (!insertion_result_addr.second) {
                    // Key already existed, value was not overwritten
                    throw CmaException("Asset Key already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }

                auto *new_asset = new cma_ledger_asset_struct_t();
                new_asset->type = asset_type;
                std::ignore = std::copy_n(std::begin(token_address->data), CMA_ABI_ADDRESS_LENGTH,
                    std::begin(new_asset->token_address.data));
                std::pair<lassid_to_asset_t::iterator, bool> insertion_result =
                    lassid_to_asset.insert({next_asset_id, *new_asset});
                if (!insertion_result.second) {
                    // shouldn't be here
                    throw CmaException("Asset ID already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }

                if (asset_id != nullptr) {
                    *asset_id = next_asset_id;
                }
                next_asset_id++;
            }
            break;
        }
        case CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID: {
            // find by address and id, create with address and id (create asset to lassid)

            // 1: look for asset
            if (token_address == nullptr) {
                throw CmaException("Invalid token address ptr", -EINVAL);
            }
            if (token_id == nullptr) {
                throw CmaException("Invalid token id ptr", -EINVAL);
            }

            cma_ledger_asset_key_bytes_t asset_key = {};
            std::span<uint8_t> asset_key_bytes_span(asset_key);
            std::span<uint8_t> asset_key_bytes_addr_span =
                asset_key_bytes_span.subspan(CMA_LEDGER_ASSET_ARRAY_KEY_ADDRESS_IND, CMA_ABI_ADDRESS_LENGTH);
            std::span<uint8_t> asset_key_bytes_id_span =
                asset_key_bytes_span.subspan(CMA_LEDGER_ASSET_ARRAY_KEY_ID_IND, CMA_ABI_ID_LENGTH);
            asset_key[CMA_LEDGER_ASSET_ARRAY_KEY_TYPE_IND] = asset_type;
            std::ignore =
                std::copy_n(std::begin(token_address->data), CMA_ABI_ADDRESS_LENGTH, asset_key_bytes_addr_span.begin());
            std::ignore = std::copy_n(std::begin(token_id->data), CMA_ABI_ID_LENGTH, asset_key_bytes_id_span.begin());

            if (operation == CMA_LEDGER_OP_FIND || operation == CMA_LEDGER_OP_FIND_OR_CREATE ||
                operation == CMA_LEDGER_OP_FIND_AND_REMOVE) {
                auto find_result_addr = asset_to_lassid.find(asset_key);
                if (find_result_addr != asset_to_lassid.end()) {
                    if (!find_asset(find_result_addr->second, &asset_type, token_address, token_id, out_total_supply)) {
                        // shouldn't be here
                        throw CmaException("Asset by id not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
                    }
                    if (asset_id != nullptr) {
                        *asset_id = find_result_addr->second;
                    }
                    if (operation == CMA_LEDGER_OP_FIND_AND_REMOVE) {
                        remove_asset(find_result_addr->second);
                    }
                    return;
                }
                if (operation == CMA_LEDGER_OP_FIND || operation == CMA_LEDGER_OP_FIND_AND_REMOVE) {
                    throw CmaException("Asset not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
                }
            }

            // 2: create asset id map (and reverse token address, token id)
            if (operation == CMA_LEDGER_OP_CREATE || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                if (next_asset_id >= max_assets) {
                    throw CmaException("Max assets reached", CMA_LEDGER_ERROR_MAX_ASSETS_REACHED);
                }
                std::pair<asset_to_lassid_t::iterator, bool> insertion_result_addr =
                    asset_to_lassid.insert({asset_key, next_asset_id});
                if (!insertion_result_addr.second) {
                    // Key already existed, value was not overwritten
                    throw CmaException("Asset Key already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }

                auto *new_asset = new cma_ledger_asset_struct_t();
                new_asset->type = asset_type;
                std::ignore = std::copy_n(std::begin(token_address->data), CMA_ABI_ADDRESS_LENGTH,
                    std::begin(new_asset->token_address.data));
                std::ignore =
                    std::copy_n(std::begin(token_id->data), CMA_ABI_ID_LENGTH, std::begin(new_asset->token_id.data));
                std::pair<lassid_to_asset_t::iterator, bool> insertion_result =
                    lassid_to_asset.insert({next_asset_id, *new_asset});
                if (!insertion_result.second) {
                    // shouldn't be here
                    throw CmaException("Asset ID already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }

                if (asset_id != nullptr) {
                    *asset_id = next_asset_id;
                }
                next_asset_id++;
            }
            break;
        }
        default:
            throw CmaException("Invalid asset type", -EINVAL);
    }
    if (m_region.get_address() != nullptr) {
        m_region.flush();
    }
}

auto cma_ledger_memory::get_account_count() -> size_t {
    return laccid_to_account.size();
}

auto cma_ledger_memory::find_account(cma_ledger_account_id_t account_id, cma_ledger_account_t *account,
    size_t *n_balances) -> bool {
    auto find_result = laccid_to_account.find(account_id);
    if (find_result == laccid_to_account.end()) {
        return false;
    }

    // account found
    if (account != nullptr) {
        account->type = find_result->second.account.type;
        std::ignore = std::copy_n(std::begin(find_result->second.account.account_id.data), CMA_ABI_ID_LENGTH,
            std::begin(account->account_id.data));
    }
    if (n_balances != nullptr) {
        *n_balances = find_result->second.n_balances;
    }

    return true;
}

auto cma_ledger_memory::remove_account(cma_ledger_account_id_t account_id) -> void {
    auto find_result = laccid_to_account.find(account_id);
    if (find_result == laccid_to_account.end()) {
        throw CmaException("Account not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
    }
    if (find_result->second.n_balances > 0) {
        throw CmaException("Account still have balances", CMA_LEDGER_ERROR_ACCOUNT_BALANCE);
    }

    switch (find_result->second.account.type) {
        case CMA_LEDGER_ACCOUNT_TYPE_ID: {
            break;
        }
        case CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS:
        case CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID: {
            cma_ledger_account_key_bytes_t account_key;
            std::ignore = std::copy_n(std::begin(find_result->second.account.account_id.data), CMA_ABI_ID_LENGTH,
                account_key.begin());
            if (account_to_laccid.erase(account_key) == 0) {
                throw CmaException("Coundn't erase account key map", CMA_LEDGER_ERROR_REMOVE);
            }
            break;
        }
        default: {
            // shouldn't be here (wrongly added to map)
            throw CmaException("Invalid account type", -EINVAL);
        }
    }

    if (laccid_to_account.erase(account_id) == 0) {
        throw CmaException("Coundn't erase account id map", CMA_LEDGER_ERROR_REMOVE);
    }
}
void cma_ledger_memory::retrieve_account(cma_ledger_account_id_t *account_id, cma_ledger_account_t *account,
    const void *addr_accid, size_t *n_balances, cma_ledger_account_type_t &account_type,
    cma_ledger_retrieve_operation_t operation) {

    switch (account_type) {
        case CMA_LEDGER_ACCOUNT_TYPE_ID: {
            // find by id, create with no account (don't create account to laccid)
            // ignores type in account struct

            // 1: look for account_id
            if (account_id == nullptr) {
                throw CmaException("Invalid account id ptr", -EINVAL);
            }
            if (operation == CMA_LEDGER_OP_FIND || operation == CMA_LEDGER_OP_FIND_OR_CREATE ||
                operation == CMA_LEDGER_OP_FIND_AND_REMOVE) {
                if (cma_ledger_memory::find_account(*account_id, account, n_balances)) {
                    if (account != nullptr) {
                        account_type = account->type;
                    }
                    if (operation == CMA_LEDGER_OP_FIND_AND_REMOVE) {
                        remove_account(*account_id);
                    }
                    return;
                }
                if (operation == CMA_LEDGER_OP_FIND || operation == CMA_LEDGER_OP_FIND_AND_REMOVE) {
                    throw CmaException("Account not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
                }
            }

            // 2: create account id map (but no reverse)
            if (operation == CMA_LEDGER_OP_CREATE || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                if (next_account_id >= max_accounts) {
                    throw CmaException("Max accounts reached", CMA_LEDGER_ERROR_MAX_ACCOUNTS_REACHED);
                }
                auto *new_account = new cma_ledger_account_struct_t();
                new_account->account.type = account_type;
                std::pair<laccid_to_account_t::iterator, bool> insertion_result =
                    laccid_to_account.insert({next_account_id, *new_account});
                if (!insertion_result.second) {
                    // shouldn't be here
                    throw CmaException("Account ID already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }

                *account_id = next_account_id;
                next_account_id++;
            }
            break;
        }
        case CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS:
        case CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID: {
            // find by wallet address, create with address (create account to laccid)

            cma_ledger_account_struct_t account_local = {};
            if (addr_accid != nullptr) {
                if (account_type == CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS) {
                    std::ignore = std::copy_n((static_cast<const cma_abi_address_t *>(addr_accid))->data,
                        CMA_ABI_ADDRESS_LENGTH, static_cast<uint8_t *>(account_local.account.address.data));
                } else if (account_type == CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID) {
                    std::ignore = std::copy_n((static_cast<const cma_account_id_t *>(addr_accid))->data,
                        CMA_ABI_ID_LENGTH, static_cast<uint8_t *>(account_local.account.account_id.data));
                }
            } else {
                if (account == nullptr) {
                    throw CmaException("Invalid account ptr", -EINVAL);
                }
                std::ignore = std::copy_n(std::begin(account->account_id.data), CMA_ABI_ID_LENGTH,
                    std::begin(account_local.account.account_id.data));
            }

            // 1: look for account
            // when using address ensure that fix bytes are zeroed
            if (account_type == CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS) {
                memset(account_local.account.fix, 0, CMA_ABI_ID_LENGTH - CMA_ABI_ADDRESS_LENGTH);
            }
            cma_ledger_account_key_bytes_t account_key;
            std::ignore =
                std::copy_n(std::begin(account_local.account.account_id.data), CMA_ABI_ID_LENGTH, account_key.begin());

            if (operation == CMA_LEDGER_OP_FIND || operation == CMA_LEDGER_OP_FIND_OR_CREATE ||
                operation == CMA_LEDGER_OP_FIND_AND_REMOVE) {
                auto find_result_acc = account_to_laccid.find(account_key);
                if (find_result_acc != account_to_laccid.end()) {
                    if (!cma_ledger_memory::find_account(find_result_acc->second, &account_local.account, n_balances)) {
                        // shouldn't be here
                        throw CmaException("Account by id not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
                    }
                    if (account_id != nullptr) {
                        *account_id = find_result_acc->second;
                    }
                    if (account != nullptr) {
                        account->type = account_local.account.type;
                        account_type = account_local.account.type;
                        std::ignore = std::copy_n(std::begin(account_local.account.account_id.data), CMA_ABI_ID_LENGTH,
                            std::begin(account->account_id.data));
                    }
                    if (operation == CMA_LEDGER_OP_FIND_AND_REMOVE) {
                        remove_account(find_result_acc->second);
                    }
                    return;
                }
                if (operation == CMA_LEDGER_OP_FIND || operation == CMA_LEDGER_OP_FIND_AND_REMOVE) {
                    throw CmaException("Account not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
                }
            }

            // 2: create account id map (and reverse account)
            if (operation == CMA_LEDGER_OP_CREATE || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                if (next_account_id >= max_accounts) {
                    throw CmaException("Max accounts reached", CMA_LEDGER_ERROR_MAX_ACCOUNTS_REACHED);
                }
                std::pair<account_to_laccid_t::iterator, bool> insertion_result_acc =
                    account_to_laccid.insert({account_key, next_account_id});
                if (!insertion_result_acc.second) {
                    // Key already existed, value was not overwritten
                    throw CmaException("Account Key already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }

                account_local.account.type = account_type; // set correct type
                std::pair<laccid_to_account_t::iterator, bool> insertion_result =
                    laccid_to_account.insert({next_account_id, account_local});
                if (!insertion_result.second) {
                    // shouldn't be here
                    throw CmaException("Account ID already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }

                if (account_id != nullptr) {
                    *account_id = next_account_id;
                }
                if (account != nullptr) {
                    account->type = account_local.account.type;
                    std::ignore = std::copy_n(std::begin(account_local.account.account_id.data), CMA_ABI_ID_LENGTH,
                        std::begin(account->account_id.data));
                }
                next_account_id++;
            }
            break;
        }
        default:
            throw CmaException("Invalid asset type", -EINVAL);
    }
    if (m_region.get_address() != nullptr) {
        m_region.flush();
    }
}

void cma_ledger_memory::get_account_asset_balance(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t account_id,
    cma_amount_t *balance, cma_ledger_account_balance_info_t *account_balance_info) {
    auto find_result = account_asset_balance.find({asset_id, account_id});

    if (balance != nullptr) {
        if (find_result == account_asset_balance.end()) {
            std::fill_n(std::begin(balance->data), CMA_ABI_U256_LENGTH, (uint8_t) 0);
        } else {
            switch (find_result->second.type) {
                case CMA_LEDGER_BALANCE_TYPE_VIRTUAL: {
                    std::ignore = std::copy_n(std::begin(find_result->second.virtual_balance->amount.data),
                        CMA_ABI_U256_LENGTH, std::begin(balance->data));
                    break;
                }
                case CMA_LEDGER_BALANCE_TYPE_WITHDRAWABLE: {
                    std::ignore = std::copy_n(std::begin(find_result->second.withdrawable_balance->amount.data),
                        CMA_ABI_U256_LENGTH, std::begin(balance->data));
                    break;
                }
                default:
                    throw CmaException("Invalid balance type", -EINVAL);
            }
        }
    }

    if (account_balance_info != nullptr) {
        if (find_result == account_asset_balance.end() ||
            find_result->second.type != CMA_LEDGER_BALANCE_TYPE_WITHDRAWABLE) {
            account_balance_info->balance = nullptr;
            return;
        }
        account_balance_info->balance = find_result->second.withdrawable_balance.get();
        account_balance_info->index = find_result->second.withdrawable_balance.get() - balances.data();
        account_balance_info->offset =
            reinterpret_cast<char *>(balances.data()) - reinterpret_cast<char *>(m_memory.get_address()) +
            account_balance_info->index * sizeof(cma_ledger_account_balance_t);
    }
}

void cma_ledger_memory::set_account_asset_balance(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t account_id,
    const cma_amount_t &balance) {
    auto find_result = account_asset_balance.find({asset_id, account_id});
    if (find_result == account_asset_balance.end()) {
        // create new entry
        if (account_asset_balance.size() >= max_balances) {
            throw CmaException("Max balances reached", CMA_LEDGER_ERROR_MAX_BALANCES_REACHED);
        }
        // check if it is withdrawable (if not create virtual balance)
        cma_balance_type_t balance_type;
        auto asset_find_result = lassid_to_asset.find(asset_id);
        if (asset_find_result == lassid_to_asset.end()) {
            throw CmaException("Asset by id not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
        }
        auto account_find_result = laccid_to_account.find(account_id);
        if (account_find_result == laccid_to_account.end()) {
            throw CmaException("Account by id not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
        }
        if (asset_find_result->second.type == CMA_LEDGER_ASSET_TYPE_ID) {
            balance_type = CMA_LEDGER_BALANCE_TYPE_VIRTUAL;
        } else {
            if (account_find_result->second.account.type != CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS) {
                balance_type = CMA_LEDGER_BALANCE_TYPE_VIRTUAL;
            } else {
                balance_type = CMA_LEDGER_BALANCE_TYPE_WITHDRAWABLE;
            }
        }

        cma_balance_t new_balance = {.type = balance_type};
        switch (balance_type) {
            case CMA_LEDGER_BALANCE_TYPE_VIRTUAL: {
                cma_ledger_account_virtual_balance_t new_virtual_balance = {};
                std::ignore = std::copy_n(std::begin(balance.data), CMA_ABI_U256_LENGTH,
                    std::begin(new_virtual_balance.amount.data));

                virtual_balances.push_back(new_virtual_balance);
                new_balance.virtual_balance = &virtual_balances.back();
                last_virtual_balances.push_back({asset_id, account_id});
                break;
            }
            case CMA_LEDGER_BALANCE_TYPE_WITHDRAWABLE: {
                cma_ledger_account_balance_t new_withdrawable_balance = {};
                // uint32_t type;
                new_withdrawable_balance.type = static_cast<uint32_t>(asset_find_result->second.type);
                // cma_abi_address_t owner;
                std::ignore = std::copy_n(std::begin(account_find_result->second.account.address.data),
                    CMA_ABI_ADDRESS_LENGTH, std::begin(new_withdrawable_balance.owner.data));

                // cma_amount_t amount;
                std::ignore = std::copy_n(std::begin(balance.data), CMA_ABI_U256_LENGTH,
                    std::begin(new_withdrawable_balance.amount.data));

                switch (asset_find_result->second.type) {
                    case CMA_LEDGER_ASSET_TYPE_ID:
                    case CMA_LEDGER_ASSET_TYPE_BASE: {
                        break;
                    }
                    case CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS: {
                        // cma_token_address_t token;
                        std::ignore = std::copy_n(std::begin(asset_find_result->second.token_address.data),
                            CMA_ABI_ADDRESS_LENGTH, std::begin(new_withdrawable_balance.token_address.data));
                        break;
                    }
                    case CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID: {
                        // cma_token_address_t token;
                        std::ignore = std::copy_n(std::begin(asset_find_result->second.token_address.data),
                            CMA_ABI_ADDRESS_LENGTH, std::begin(new_withdrawable_balance.token_address.data));
                        // cma_token_id_t token_id;
                        std::ignore = std::copy_n(std::begin(asset_find_result->second.token_id.data),
                            CMA_ABI_ID_LENGTH, std::begin(new_withdrawable_balance.token_id.data));
                        break;
                    }
                    default: {
                        // shouldn't be here (wrongly added to map)
                        throw CmaException("Invalid asset type", -EINVAL);
                    }
                }

                balances.push_back(new_withdrawable_balance);
                new_balance.withdrawable_balance = &balances.back();
                last_balances.push_back({asset_id, account_id});
                break;
            }
            default:
                throw CmaException("Invalid balance type", -EINVAL);
        }

        cma_map_key_t balance_key = {asset_id, account_id};
        auto insertion_result = account_asset_balance.emplace(balance_key, new_balance);
        if (!insertion_result.second) {
            // shouldn't be here
            throw CmaException("Balance already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
        }
        account_find_result->second.n_balances++;

        return;
    }

    auto no_balance = is_zero(balance);

    switch (find_result->second.type) {
        case CMA_LEDGER_BALANCE_TYPE_VIRTUAL: {
            std::ignore = std::copy_n(std::begin(balance.data), CMA_ABI_U256_LENGTH,
                std::begin(find_result->second.virtual_balance->amount.data));
            if (no_balance) {
                if (account_asset_balance.size() > 1) {
                    // copy last balance from list to current position
                    if (last_virtual_balances.empty()) {
                        // shouldn't be here
                        throw CmaException("Last virtual balance key not found", CMA_LEDGER_ERROR_BALANCE_NOT_FOUND);
                    }
                    auto last_virtual_balance = last_virtual_balances.back();
                    auto find_result_last = account_asset_balance.find(last_virtual_balance);
                    if (find_result_last == account_asset_balance.end()) {
                        // shouldn't be here
                        throw CmaException("Last virtual balance not found", CMA_LEDGER_ERROR_BALANCE_NOT_FOUND);
                    }
                    std::ignore = std::copy_n(std::begin(find_result_last->second.virtual_balance->amount.data),
                        CMA_ABI_U256_LENGTH, std::begin(find_result->second.virtual_balance->amount.data));
                    last_virtual_balances.pop_back();
                }
                auto account_find_result = laccid_to_account.find(account_id);
                if (account_find_result == laccid_to_account.end()) {
                    throw CmaException("Account by id not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
                }
                account_find_result->second.n_balances--;
                virtual_balances.pop_back();
                if (account_asset_balance.erase(find_result->first) == 0) {
                    throw CmaException("Coundn't erase last virtual balance", CMA_LEDGER_ERROR_REMOVE);
                }
            }
            break;
        }
        case CMA_LEDGER_BALANCE_TYPE_WITHDRAWABLE: {
            std::ignore = std::copy_n(std::begin(balance.data), CMA_ABI_U256_LENGTH,
                std::begin(find_result->second.withdrawable_balance->amount.data));
            if (no_balance) {
                std::ignore = std::fill_n(reinterpret_cast<uint8_t *>(find_result->second.withdrawable_balance.get()),
                    sizeof(cma_ledger_account_balance_t), (uint8_t) 0);
                if (account_asset_balance.size() > 1) {
                    // copy last balance from list to current position
                    if (last_balances.empty()) {
                        // shouldn't be here
                        throw CmaException("Last balance key not found", CMA_LEDGER_ERROR_BALANCE_NOT_FOUND);
                    }
                    auto last_balance = last_balances.back();
                    auto find_result_last = account_asset_balance.find(last_balance);
                    if (find_result_last == account_asset_balance.end()) {
                        // shouldn't be here
                        throw CmaException("Last balance not found", CMA_LEDGER_ERROR_BALANCE_NOT_FOUND);
                    }
                    find_result->second.withdrawable_balance->type =
                        find_result_last->second.withdrawable_balance->type;
                    std::ignore = std::copy_n(std::begin(find_result_last->second.withdrawable_balance->owner.data),
                        CMA_ABI_ADDRESS_LENGTH, std::begin(find_result->second.withdrawable_balance->owner.data));
                    std::ignore =
                        std::copy_n(std::begin(find_result_last->second.withdrawable_balance->token_address.data),
                            CMA_ABI_ADDRESS_LENGTH,
                            std::begin(find_result->second.withdrawable_balance->token_address.data));
                    std::ignore = std::copy_n(std::begin(find_result_last->second.withdrawable_balance->token_id.data),
                        CMA_ABI_ID_LENGTH, std::begin(find_result->second.withdrawable_balance->token_id.data));
                    std::ignore = std::copy_n(std::begin(find_result_last->second.withdrawable_balance->amount.data),
                        CMA_ABI_U256_LENGTH, std::begin(find_result->second.withdrawable_balance->amount.data));
                    std::ignore =
                        std::fill_n(reinterpret_cast<uint8_t *>(find_result_last->second.withdrawable_balance.get()),
                            sizeof(cma_ledger_account_balance_t), (uint8_t) 0);
                    find_result_last->second.withdrawable_balance = find_result->second.withdrawable_balance;
                    last_balances.pop_back();
                }
                auto account_find_result = laccid_to_account.find(account_id);
                if (account_find_result == laccid_to_account.end()) {
                    throw CmaException("Account by id not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
                }
                account_find_result->second.n_balances--;

                balances.pop_back();
                if (account_asset_balance.erase(find_result->first) == 0) {
                    throw CmaException("Coundn't erase last balance", CMA_LEDGER_ERROR_REMOVE);
                }
            }
            break;
        }
        default: {
            throw CmaException("Invalid balance type", -EINVAL);
        }
    }
}

void cma_ledger_memory::deposit(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t to_account_id,
    const cma_amount_t &deposit) {
    // 1: check asset
    cma_amount_t curr_supply = {};
    if (!find_asset(asset_id, nullptr, nullptr, nullptr, &curr_supply)) {
        throw CmaException("Asset by id not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
    }

    cma_amount_t new_supply = {};
    if (!amount_checked_add(new_supply, curr_supply, deposit)) {
        throw CmaException("Asset supply overflow", CMA_LEDGER_ERROR_SUPPLY_OVERFLOW);
    }
    // 2: check account
    if (!cma_ledger_memory::find_account(to_account_id, nullptr, nullptr)) {
        throw CmaException("Account by id not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
    }
    // 3: check amount
    cma_amount_t curr_balance = {};
    get_account_asset_balance(asset_id, to_account_id, &curr_balance, nullptr);

    cma_amount_t new_balance = {};
    if (!amount_checked_add(new_balance, curr_balance, deposit)) {
        throw CmaException("Asset balance overflow", CMA_LEDGER_ERROR_BALANCE_OVERFLOW);
    }

    // 4: update account balance
    set_account_asset_balance(asset_id, to_account_id, new_balance);

    // 5: update asset supply
    set_asset_supply(asset_id, new_supply);

    // 6: flush
    if (m_region.get_address() != nullptr) {
        m_region.flush();
    }
}

void cma_ledger_memory::withdraw(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t from_account_id,
    const cma_amount_t &withdrawal) {
    // 1: check asset
    cma_amount_t curr_supply = {};
    if (!find_asset(asset_id, nullptr, nullptr, nullptr, &curr_supply)) {
        throw CmaException("Asset by id not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
    }

    cma_amount_t new_supply = {};
    if (!amount_checked_sub(new_supply, curr_supply, withdrawal)) {
        throw CmaException("Asset supply underflow", CMA_LEDGER_ERROR_SUPPLY_OVERFLOW);
    }
    // 2: check account
    if (!cma_ledger_memory::find_account(from_account_id, nullptr, nullptr)) {
        throw CmaException("Account by id not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
    }
    // 3: check amount
    cma_amount_t curr_balance = {};
    get_account_asset_balance(asset_id, from_account_id, &curr_balance, nullptr);

    cma_amount_t new_balance = {};
    if (!amount_checked_sub(new_balance, curr_balance, withdrawal)) {
        throw CmaException("Insufficient funds", CMA_LEDGER_ERROR_INSUFFICIENT_FUNDS);
    }

    // 4: update account balance
    set_account_asset_balance(asset_id, from_account_id, new_balance);

    // 5: update asset supply
    set_asset_supply(asset_id, new_supply);

    // 6: flush
    if (m_region.get_address() != nullptr) {
        m_region.flush();
    }

    // TODO: cleanup asset with not supply and account with no balance
}

void cma_ledger_memory::transfer(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t from_account_id,
    cma_ledger_account_id_t to_account_id, const cma_amount_t &amount) {
    // 1: check asset
    cma_amount_t curr_supply = {};
    if (!find_asset(asset_id, nullptr, nullptr, nullptr, &curr_supply)) {
        throw CmaException("Asset by id not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
    }

    // 2: check account from
    if (from_account_id == to_account_id) {
        throw CmaException("Account from equal to account to", -EINVAL);
    }
    if (!cma_ledger_memory::find_account(from_account_id, nullptr, nullptr)) {
        throw CmaException("Account from not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
    }
    // 3: check amount from
    cma_amount_t curr_balance_from = {};
    get_account_asset_balance(asset_id, from_account_id, &curr_balance_from, nullptr);

    cma_amount_t new_balance_from = {};
    if (!amount_checked_sub(new_balance_from, curr_balance_from, amount)) {
        throw CmaException("Insufficient funds", CMA_LEDGER_ERROR_INSUFFICIENT_FUNDS);
    }

    // 4: check account to
    if (!cma_ledger_memory::find_account(to_account_id, nullptr, nullptr)) {
        throw CmaException("Account to not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
    }
    // 5: check amount to
    cma_amount_t curr_balance_to = {};
    get_account_asset_balance(asset_id, to_account_id, &curr_balance_to, nullptr);

    cma_amount_t new_balance_to = {};
    if (!amount_checked_add(new_balance_to, curr_balance_to, amount)) {
        throw CmaException("Balance overflow", CMA_LEDGER_ERROR_BALANCE_OVERFLOW);
    }

    // 6: update account balances
    set_account_asset_balance(asset_id, from_account_id, new_balance_from);
    set_account_asset_balance(asset_id, to_account_id, new_balance_to);

    // 6: flush
    if (m_region.get_address() != nullptr) {
        m_region.flush();
    }
}

auto cma_ledger_memory::get_balances() -> balance_list_t * {
    return &balances;
}

auto cma_ledger_memory::get_memory_address() -> void * {
    return m_memory.get_address();
}
