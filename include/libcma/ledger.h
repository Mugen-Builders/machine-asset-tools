#ifndef CMA_LEDGER_H
#define CMA_LEDGER_H
#include <stdint.h>

#include "types.h"

typedef struct cma_ledger_impl cma_ledger_t;

typedef uint64_t cma_ledger_account_id_t;
typedef uint64_t cma_ledger_asset_id_t;

typedef enum {
    CMA_LEDGER_SUCCESS = 0,
    CMA_LEDGER_ERROR_INSUFFICIENT_FUNDS = -1001,
    CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND = -1002,
    CMA_LEDGER_ERROR_ASSET_NOT_FOUND = -1003,
    CMA_LEDGER_ERROR_ACCOUNTS_LIMIT = -1004,
    CMA_LEDGER_ERROR_ASSETS_LIMIT = -1005,
    CMA_LEDGER_ERROR_INVALID_ACCOUNT = -1006,
    CMA_LEDGER_ERROR_UNKNOWN = -1007
} cma_ledger_error_t;

enum cma_ledger_account_creation_type_t {
    CMA_LEDGER_ACCOUNT_CREATION_TYPE_SEQUENTIAL,
    CMA_LEDGER_ACCOUNT_CREATION_TYPE_ACCOUNT,
    CMA_LEDGER_ACCOUNT_CREATION_TYPE_WALLET,
};

struct cma_ledger_account_t {
    cma_ledger_account_id_t id;
    union {
        cma_account_id_t account_id;
        struct {
            uint8_t fix[CMT_ABI_U256_LENGTH - CMT_ABI_ADDRESS_LENGTH];
            cma_abi_address_t address;
        };
    };
};

struct cma_ledger_asset_t {
    cma_ledger_asset_id_t id;
    cma_token_address_t token_address;
    cma_token_id_t token_id;
};

cma_ledger_error_t ledger_init(cma_ledger_t *ledger);
cma_ledger_error_t ledger_reset(cma_ledger_t *ledger);
cma_ledger_error_t ledger_fini(cma_ledger_t *ledger);

cma_ledger_error_t cma_ledger_load(cma_ledger_t *ledger, const char *filepath);
cma_ledger_error_t cma_ledger_save(cma_ledger_t *ledger, const char *filepath);

// Create a new asset for the given token and
// update the token (address, token id) ->
// asset_id mapping
cma_ledger_error_t ledger_asset_create(cma_ledger_t *ledger, cma_ledger_asset_t *asset);

// Retrieve an asset
// If id is defined, fill with the asset details, otherwise fill with id
cma_ledger_error_t ledger_asset_retrieve(const cma_ledger_t *ledger, cma_ledger_asset_t *asset);

// Create a new account for the given account
// and update the account -> account_id mapping (if any)
cma_ledger_error_t ledger_account_create(cma_ledger_t *ledger, cma_ledger_account_creation_type_t creation_type,
    cma_ledger_account_t *account);

// Retrieve an account
// If id is defined, fill with the account details, otherwise fill with id
cma_ledger_error_t ledger_account_retrieve(const cma_ledger_t *ledger, cma_ledger_account_t *account);

// Deposit
cma_ledger_error_t ledger_deposit(cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id,
    cma_ledger_account_id_t to_account_id, const cma_amount_t *amount);

// Withdrawal
cma_ledger_error_t ledger_withdrawal(cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id,
    cma_ledger_account_id_t from_account_id, const cma_amount_t *amount);

// Transfer
cma_ledger_error_t ledger_transfer(cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id,
    cma_ledger_account_id_t from_account_id, cma_ledger_account_id_t to_account_id, const cma_amount_t *amount);

// Get balance
cma_ledger_error_t ledger_get_balance(const cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id,
    cma_ledger_account_id_t account_id, cma_amount_t *out_balance);

// Get total supply
cma_ledger_error_t ledger_get_total_supply(const cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id,
    cma_amount_t *out_total_supply);

// get error message
const char *cma_ledger_get_error_message(cma_ledger_error_t error);

#endif // CMA_LEDGER_H
