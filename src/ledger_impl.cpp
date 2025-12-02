#include <algorithm>
#include <cerrno>
#include <cstdio>

extern "C" {
#include "libcma/ledger.h"
}

#include "ledger_impl.h"
#include "utils.h"

/*
 * Ledger class
 */

cma_ledger::~cma_ledger() {
    magic = 0;
}

auto cma_ledger::is_initialized() const -> bool {
    return magic == CMA_LEDGER_MAGIC;
}

void cma_ledger::clear() {
    account_to_laccid.clear();
    laccid_to_account.clear();
    asset_to_lassid.clear();
    lassid_to_asset.clear();
    account_asset_balance.clear();
}

auto cma_ledger::get_asset_count() -> size_t {
    return lassid_to_asset.size();
}

auto cma_ledger::find_asset(cma_ledger_asset_id_t asset_id, cma_token_address_t *token_address,
    cma_token_id_t *token_id, cma_amount_t *supply) -> bool {
    lassid_to_asset_t::iterator find_result = lassid_to_asset.find(asset_id);
    if (find_result == lassid_to_asset.end()) {
        return false;
    }

    // asset found
    switch (find_result->second.type) {
        case CMA_LEDGER_ASSET_TYPE_ID:
            break;
        case CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS:
            if (token_address != nullptr)
                std::ignore = std::copy(find_result->second.token_address.data,
                    find_result->second.token_address.data + CMA_ABI_ADDRESS_LENGTH, token_address->data);
            break;
        case CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID:
            if (token_address != nullptr)
                std::ignore = std::copy(find_result->second.token_address.data,
                    find_result->second.token_address.data + CMA_ABI_ADDRESS_LENGTH, token_address->data);
            if (token_id != nullptr)
                std::ignore = std::copy(find_result->second.token_id.data,
                    find_result->second.token_id.data + CMA_ABI_ID_LENGTH, token_id->data);
            break;
        default:
            // shouldn't be here (wrongly added to map)
            throw LedgerException("Invalid asset type", -EINVAL);
    }
    if (supply != nullptr) {
        std::ignore = std::copy(find_result->second.supply.data, find_result->second.supply.data + CMA_ABI_U256_LENGTH,
            supply->data);
    }
    return true;
}

void cma_ledger::set_asset_supply(cma_ledger_asset_id_t asset_id, cma_amount_t &supply) {
    lassid_to_asset_t::iterator find_result = lassid_to_asset.find(asset_id);
    if (find_result == lassid_to_asset.end()) {
        throw LedgerException("Asset by id not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
    }

    std::ignore = std::copy(supply.data, supply.data + CMA_ABI_U256_LENGTH, find_result->second.supply.data);
}

void cma_ledger::retrieve_create_asset(cma_ledger_asset_id_t *asset_id, cma_token_address_t *token_address,
    cma_token_id_t *token_id, cma_ledger_asset_type_t asset_type, cma_ledger_retrieve_operation_t operation) {

    const size_t curr_size = get_asset_count();

    switch (asset_type) {
        case CMA_LEDGER_ASSET_TYPE_ID: {
            // find by id, create with no address and id (don't create asset to lassid)

            // 1: look for asset
            if (asset_id == nullptr) {
                throw LedgerException("Invalid asset id ptr", -EINVAL);
            }
            if (operation == CMA_LEDGER_OP_FIND || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                if (find_asset(*asset_id, token_address, token_id, nullptr)) {
                    return;
                } else if (operation == CMA_LEDGER_OP_FIND) {
                    throw LedgerException("Asset not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
                }
            }

            // 2: create asset id map (but no reverse)
            if (operation == CMA_LEDGER_OP_CREATE || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                cma_ledger_asset_struct_t *new_asset = new cma_ledger_asset_struct_t();
                new_asset->type = CMA_LEDGER_ASSET_TYPE_ID;
                std::pair<lassid_to_asset_t::iterator, bool> insertion_result =
                    lassid_to_asset.insert({curr_size, *new_asset});
                if (!insertion_result.second) {
                    // Key already existed, value was not overwritten
                    // shouldn't be here
                    throw LedgerException("Asset ID already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }

                *asset_id = curr_size;
                delete new_asset;
            }
            break;
        }
        case CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS: {
            // find by address, create with address (create asset to lassid)

            // 1: look for asset
            if (token_address == nullptr) {
                throw LedgerException("Invalid token address ptr", -EINVAL);
            }

            cma_ledger_asset_key_bytes_t asset_key_bytes = {};
            asset_key_bytes[CMA_LEDGER_ASSET_ARRAY_KEY_TYPE_IND] = CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS;
            std::ignore = std::copy(token_address->data, token_address->data + CMA_ABI_ADDRESS_LENGTH,
                asset_key_bytes + CMA_LEDGER_ASSET_ARRAY_KEY_ADDRESS_IND);
            size_t asset_key_bytes_len = sizeof(asset_key_bytes) / sizeof(asset_key_bytes[0]);
            cma_ledger_asset_key_t asset_key(reinterpret_cast<const char *>(asset_key_bytes), asset_key_bytes_len);

            if (operation == CMA_LEDGER_OP_FIND || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                asset_to_lassid_t::iterator find_result_addr = asset_to_lassid.find(asset_key);
                if (find_result_addr != asset_to_lassid.end()) {
                    if (!find_asset(find_result_addr->second, token_address, token_id, nullptr)) {
                        // shouldn't be here
                        throw LedgerException("Asset by id not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
                    }
                    if (asset_id != nullptr) {
                        *asset_id = find_result_addr->second;
                    }
                    return;
                } else if (operation == CMA_LEDGER_OP_FIND) {
                    throw LedgerException("Asset not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
                }
            }

            // 2: create asset id map (and reverse token address, no token id)
            if (operation == CMA_LEDGER_OP_CREATE || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                std::pair<asset_to_lassid_t::iterator, bool> insertion_result_addr =
                    asset_to_lassid.insert({asset_key, curr_size});
                if (!insertion_result_addr.second) {
                    // Key already existed, value was not overwritten
                    throw LedgerException("Asset Key already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }

                cma_ledger_asset_struct_t *new_asset = new cma_ledger_asset_struct_t();
                new_asset->type = CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS;
                std::ignore = std::copy(token_address->data, token_address->data + CMA_ABI_ADDRESS_LENGTH,
                    new_asset->token_address.data);
                std::pair<lassid_to_asset_t::iterator, bool> insertion_result =
                    lassid_to_asset.insert({curr_size, *new_asset});
                if (!insertion_result.second) {
                    // shouldn't be here
                    throw LedgerException("Asset ID already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }

                if (asset_id != nullptr) {
                    *asset_id = curr_size;
                }
                delete new_asset;
            }
            break;
        }
        case CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID: {
            // find by address and id, create with address and id (create asset to lassid)

            // 1: look for asset
            if (token_address == nullptr) {
                throw LedgerException("Invalid token address ptr", -EINVAL);
            }
            if (token_id == nullptr) {
                throw LedgerException("Invalid token id ptr", -EINVAL);
            }

            cma_ledger_asset_key_bytes_t asset_key_bytes = {};
            asset_key_bytes[CMA_LEDGER_ASSET_ARRAY_KEY_TYPE_IND] = CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS;
            std::ignore = std::copy(token_address->data, token_address->data + CMA_ABI_ADDRESS_LENGTH,
                asset_key_bytes + CMA_LEDGER_ASSET_ARRAY_KEY_ADDRESS_IND);
            std::ignore = std::copy(token_id->data, token_id->data + CMA_ABI_ID_LENGTH,
                asset_key_bytes + CMA_LEDGER_ASSET_ARRAY_KEY_ID_IND);
            size_t asset_key_bytes_len = sizeof(asset_key_bytes) / sizeof(asset_key_bytes[0]);
            cma_ledger_asset_key_t asset_key(reinterpret_cast<const char *>(asset_key_bytes), asset_key_bytes_len);

            if (operation == CMA_LEDGER_OP_FIND || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                asset_to_lassid_t::iterator find_result_addr = asset_to_lassid.find(asset_key);
                if (find_result_addr != asset_to_lassid.end()) {
                    if (!find_asset(find_result_addr->second, token_address, token_id, nullptr)) {
                        // shouldn't be here
                        throw LedgerException("Asset by id not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
                    }
                    if (asset_id != nullptr) {
                        *asset_id = find_result_addr->second;
                    }
                    return;
                } else if (operation == CMA_LEDGER_OP_FIND) {
                    throw LedgerException("Asset not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
                }
            }

            // 2: create asset id map (and reverse token address, token id)
            if (operation == CMA_LEDGER_OP_CREATE || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                std::pair<asset_to_lassid_t::iterator, bool> insertion_result_addr =
                    asset_to_lassid.insert({asset_key, curr_size});
                if (!insertion_result_addr.second) {
                    // Key already existed, value was not overwritten
                    throw LedgerException("Asset Key already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }

                cma_ledger_asset_struct_t *new_asset = new cma_ledger_asset_struct_t();
                new_asset->type = CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID;
                std::ignore = std::copy(token_address->data, token_address->data + CMA_ABI_ADDRESS_LENGTH,
                    new_asset->token_address.data);
                std::ignore = std::copy(token_id->data, token_id->data + CMA_ABI_ID_LENGTH, new_asset->token_id.data);
                std::pair<lassid_to_asset_t::iterator, bool> insertion_result =
                    lassid_to_asset.insert({curr_size, *new_asset});
                if (!insertion_result.second) {
                    // shouldn't be here
                    throw LedgerException("Asset ID already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }

                if (asset_id != nullptr) {
                    *asset_id = curr_size;
                }
                delete new_asset;
            }
            break;
        }
        default:
            throw LedgerException("Invalid asset type", -EINVAL);
    }
}

auto cma_ledger::get_account_count() -> size_t {
    return laccid_to_account.size();
}

auto cma_ledger::find_account(cma_ledger_account_id_t account_id, cma_ledger_account_t *account) -> bool {
    laccid_to_account_t::iterator find_result = laccid_to_account.find(account_id);
    if (find_result == laccid_to_account.end()) {
        return false;
    }

    // asset found
    if (account != nullptr) {
        account->type = find_result->second.type;
        std::ignore = std::copy(find_result->second.account_id.data,
            find_result->second.account_id.data + CMA_ABI_ID_LENGTH, account->account_id.data);
    }
    return true;
}

void cma_ledger::retrieve_create_account(cma_ledger_account_id_t *account_id, cma_ledger_account_t *account,
    const void *addr_accid, cma_ledger_account_type_t account_type, cma_ledger_retrieve_operation_t operation) {
    const size_t curr_size = get_account_count();

    switch (account_type) {
        case CMA_LEDGER_ACCOUNT_TYPE_ID: {
            // find by id, create with no account (don't create account to laccid)
            // ignores type in account struct

            // 1: look for account_id
            if (account_id == nullptr) {
                throw LedgerException("Invalid account id ptr", -EINVAL);
            }
            if (operation == CMA_LEDGER_OP_FIND || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                bool account_found = cma_ledger::find_account(*account_id, account);
                if (account_found) {
                    return;
                } else if (operation == CMA_LEDGER_OP_FIND) {
                    throw LedgerException("Account not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
                }
            }

            // 2: create account id map (but no reverse)
            if (operation == CMA_LEDGER_OP_CREATE || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                cma_ledger_account_t *new_account = new cma_ledger_account_t();
                new_account->type = CMA_LEDGER_ACCOUNT_TYPE_ID;
                std::pair<laccid_to_account_t::iterator, bool> insertion_result =
                    laccid_to_account.insert({curr_size, *new_account});
                if (!insertion_result.second) {
                    // shouldn't be here
                    throw LedgerException("Account ID already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }

                *account_id = curr_size;
                delete new_account;
            }
            break;
        }
        case CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS:
        case CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID: {
            // find by wallet address, create with address (create account to laccid)

            cma_ledger_account_t account_local;
            if (addr_accid != nullptr) {
                if (account_type == CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS) {
                    std::ignore = std::copy((static_cast<const cma_token_address_t *>(addr_accid))->data,
                        (static_cast<const cma_token_address_t *>(addr_accid))->data + CMA_ABI_ADDRESS_LENGTH,
                        account_local.address.data);
                } else if (account_type == CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID) {
                    std::ignore = std::copy((static_cast<const cma_token_id_t *>(addr_accid))->data,
                        (static_cast<const cma_token_id_t *>(addr_accid))->data + CMA_ABI_ID_LENGTH,
                        account_local.account_id.data);
                }
            } else {
                if (account == nullptr) {
                    throw LedgerException("Invalid account ptr", -EINVAL);
                }
                std::ignore = std::copy(account->account_id.data, account->account_id.data + CMA_ABI_ID_LENGTH,
                    account_local.account_id.data);
            }

            // 1: look for account
            // when using address ensure that fix bytes are zeroed
            if (account_type == CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS) {
                memset(account_local.fix, 0, CMT_ABI_U256_LENGTH - CMT_ABI_ADDRESS_LENGTH);
            }
            cma_ledger_account_key_t account_key(reinterpret_cast<const char *>(account_local.account_id.data),
                CMA_ABI_ID_LENGTH);

            if (operation == CMA_LEDGER_OP_FIND || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                account_to_laccid_t::iterator find_result_acc = account_to_laccid.find(account_key);
                if (find_result_acc != account_to_laccid.end()) {
                    if (!cma_ledger::find_account(find_result_acc->second, &account_local)) {
                        // shouldn't be here
                        throw LedgerException("Account by id not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
                    }
                    if (account_id != nullptr) {
                        *account_id = find_result_acc->second;
                    }
                    if (account != nullptr) {
                        account->type = account_local.type;
                        std::ignore = std::copy(account_local.account_id.data,
                            account_local.account_id.data + CMA_ABI_ID_LENGTH, account->account_id.data);
                    }
                    return;
                } else if (operation == CMA_LEDGER_OP_FIND) {
                    throw LedgerException("Account not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
                }
            }

            // 2: create account id map (and reverse account)
            if (operation == CMA_LEDGER_OP_CREATE || operation == CMA_LEDGER_OP_FIND_OR_CREATE) {
                std::pair<account_to_laccid_t::iterator, bool> insertion_result_acc =
                    account_to_laccid.insert({account_key, curr_size});
                if (!insertion_result_acc.second) {
                    // Key already existed, value was not overwritten
                    throw LedgerException("Account Key already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }

                account_local.type = account_type; // set correct type
                std::pair<laccid_to_account_t::iterator, bool> insertion_result =
                    laccid_to_account.insert({curr_size, account_local});
                if (!insertion_result.second) {
                    // shouldn't be here
                    throw LedgerException("Account ID already exists", CMA_LEDGER_ERROR_INSERTION_ERROR);
                }

                if (account_id != nullptr) {
                    *account_id = curr_size;
                }
                if (account != nullptr) {
                    account->type = account_local.type;
                    std::ignore = std::copy(account_local.account_id.data,
                        account_local.account_id.data + CMA_ABI_ID_LENGTH, account->account_id.data);
                }
            }
            break;
        }
        default:
            throw LedgerException("Invalid asset type", -EINVAL);
    }
}

void cma_ledger::get_account_asset_balance(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t to_account_id,
    cma_amount_t &balance) {
    account_asset_map_t::iterator find_result = account_asset_balance.find({asset_id, to_account_id});
    if (find_result == account_asset_balance.end()) {
        std::fill(balance.data, balance.data + CMA_ABI_U256_LENGTH, (uint8_t) 0);
        return;
    }

    std::ignore = std::copy(find_result->second.data, find_result->second.data + CMA_ABI_U256_LENGTH, balance.data);
}

void cma_ledger::set_account_asset_balance(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t to_account_id,
    const cma_amount_t &balance) {
    account_asset_map_t::iterator find_result = account_asset_balance.find({asset_id, to_account_id});
    if (find_result == account_asset_balance.end()) {
        // create new entry
        account_asset_balance.insert({{asset_id, to_account_id}, balance});
        return;
    }

    std::ignore = std::copy(balance.data, balance.data + CMA_ABI_U256_LENGTH, find_result->second.data);
}

void cma_ledger::deposit(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t to_account_id,
    const cma_amount_t &deposit) {
    // 1: check asset
    cma_amount_t curr_supply = {};
    if (!find_asset(asset_id, nullptr, nullptr, &curr_supply)) {
        throw LedgerException("Asset by id not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
    }

    cma_amount_t new_supply = {};
    if (!amount_checked_add(new_supply, curr_supply, deposit)) {
        throw LedgerException("Asset supply overflow", CMA_LEDGER_ERROR_SUPPLY_OVERFLOW);
    }
    // 2: check account
    if (!cma_ledger::find_account(to_account_id, nullptr)) {
        throw LedgerException("Account by id not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
    }
    // 3: check amount
    cma_amount_t curr_balance = {};
    get_account_asset_balance(asset_id, to_account_id, curr_balance);

    cma_amount_t new_balance = {};
    if (!amount_checked_add(new_balance, curr_balance, deposit)) {
        throw LedgerException("Asset balance overflow", CMA_LEDGER_ERROR_BALANCE_OVERFLOW);
    }

    // 4: update account balance
    set_account_asset_balance(asset_id, to_account_id, new_balance);

    // 5: update asset supply
    set_asset_supply(asset_id, new_supply);
}

void cma_ledger::withdraw(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t from_account_id,
    const cma_amount_t &withdrawal) {
    // 1: check asset
    cma_amount_t curr_supply = {};
    if (!find_asset(asset_id, nullptr, nullptr, &curr_supply)) {
        throw LedgerException("Asset by id not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
    }

    cma_amount_t new_supply = {};
    if (!amount_checked_sub(new_supply, curr_supply, withdrawal)) {
        throw LedgerException("Asset supply underflow", CMA_LEDGER_ERROR_SUPPLY_OVERFLOW);
    }
    // 2: check account
    if (!cma_ledger::find_account(from_account_id, nullptr)) {
        throw LedgerException("Account by id not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
    }
    // 3: check amount
    cma_amount_t curr_balance = {};
    get_account_asset_balance(asset_id, from_account_id, curr_balance);

    cma_amount_t new_balance = {};
    if (!amount_checked_sub(new_balance, curr_balance, withdrawal)) {
        throw LedgerException("Insufficient funds", CMA_LEDGER_ERROR_INSUFFICIENT_FUNDS);
    }

    // 4: update account balance
    set_account_asset_balance(asset_id, from_account_id, new_balance);

    // 5: update asset supply
    set_asset_supply(asset_id, new_supply);
}

void cma_ledger::transfer(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t from_account_id,
    cma_ledger_account_id_t to_account_id, const cma_amount_t &amount) {
    // 1: check asset
    cma_amount_t curr_supply = {};
    if (!find_asset(asset_id, nullptr, nullptr, &curr_supply)) {
        throw LedgerException("Asset by id not found", CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
    }

    // 2: check account from
    if (from_account_id == to_account_id) {
        throw LedgerException("Account from equal to account to", -EINVAL);
    }
    if (!cma_ledger::find_account(from_account_id, nullptr)) {
        throw LedgerException("Account from not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
    }
    // 3: check amount from
    cma_amount_t curr_balance_from = {};
    get_account_asset_balance(asset_id, from_account_id, curr_balance_from);

    cma_amount_t new_balance_from = {};
    if (!amount_checked_sub(new_balance_from, curr_balance_from, amount)) {
        throw LedgerException("Insufficient funds", CMA_LEDGER_ERROR_INSUFFICIENT_FUNDS);
    }

    // 4: check account to
    if (!cma_ledger::find_account(to_account_id, nullptr)) {
        throw LedgerException("Account to not found", CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
    }
    // 5: check amount to
    cma_amount_t curr_balance_to = {};
    get_account_asset_balance(asset_id, to_account_id, curr_balance_to);

    cma_amount_t new_balance_to = {};
    if (!amount_checked_add(new_balance_to, curr_balance_to, amount)) {
        throw LedgerException("Balance overflow", CMA_LEDGER_ERROR_BALANCE_OVERFLOW);
    }

    // 6: update account balances
    set_account_asset_balance(asset_id, from_account_id, new_balance_from);
    set_account_asset_balance(asset_id, to_account_id, new_balance_to);
}
