#ifndef CMA_LEDGER_H
#define CMA_LEDGER_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Compiler visibility definition
#ifndef CMA_LEDGER_API
#define CMA_LEDGER_API __attribute__((visibility("default")))
#endif

#include "types.h"

enum {
    CMA_LEDGER_T_SIZE = 328 / 8,
};

typedef struct cma_ledger_struct {
    uint64_t __opaque[CMA_LEDGER_T_SIZE];
} cma_ledger_t;

typedef uint64_t cma_ledger_account_id_t;
typedef uint64_t cma_ledger_asset_id_t;

enum {
    CMA_LEDGER_SUCCESS = 0,
    CMA_LEDGER_ERROR_UNKNOWN = -1001,
    CMA_LEDGER_ERROR_EXCEPTION = -1002,
    CMA_LEDGER_ERROR_INSUFFICIENT_FUNDS = -1003,
    CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND = -1004,
    CMA_LEDGER_ERROR_ASSET_NOT_FOUND = -1005,
    CMA_LEDGER_ERROR_SUPPLY_OVERFLOW = -1006,
    CMA_LEDGER_ERROR_BALANCE_OVERFLOW = -1007,
    CMA_LEDGER_ERROR_INVALID_ACCOUNT = -1008,
    CMA_LEDGER_ERROR_INSERTION_ERROR = -1009,
};

typedef enum : uint8_t {
    CMA_LEDGER_OP_FIND,
    CMA_LEDGER_OP_CREATE,
    CMA_LEDGER_OP_FIND_OR_CREATE,
} cma_ledger_retrieve_operation_t;

typedef enum : uint8_t {
    CMA_LEDGER_ASSET_TYPE_ID,
    CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS,
    CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID,
} cma_ledger_asset_type_t;

typedef enum : uint8_t {
    CMA_LEDGER_ACCOUNT_TYPE_ID,
    CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS,
    CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID,
} cma_ledger_account_type_t;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
typedef struct cma_ledger_account {
    cma_ledger_account_type_t type;
    union {
        cma_account_id_t account_id;
        struct {
            uint8_t fix[CMT_ABI_U256_LENGTH - CMT_ABI_ADDRESS_LENGTH];
            cma_abi_address_t address;
        } __attribute__((__packed__));
    };
} cma_ledger_account_t;
#pragma GCC diagnostic pop

CMA_LEDGER_API int cma_ledger_init(cma_ledger_t *ledger);
CMA_LEDGER_API int cma_ledger_fini(cma_ledger_t *ledger);
CMA_LEDGER_API int cma_ledger_reset(cma_ledger_t *ledger);

// TODO() int cma_ledger_load(cma_ledger_t *ledger, const char *filepath);
// TODO() int cma_ledger_save(cma_ledger_t *ledger, const char *filepath);

// Retrieve/create an asset
// try to retrieve: If id is defined, fill with the asset details, otherwise fill with id
// If it didn't find  the asset and creation type is set with one of the options, create it
// update the token asset_id <-> (address, token id) mapping
CMA_LEDGER_API int cma_ledger_retrieve_asset(cma_ledger_t *ledger, cma_ledger_asset_id_t *asset_id,
    cma_token_address_t *token_address, cma_token_id_t *token_id, cma_ledger_asset_type_t asset_type,
    cma_ledger_retrieve_operation_t op);

// Retrieve/create an account
// try to retrieve: If id is defined, fill with the account details, otherwise fill with id
// If it didn't find  the account and creation type is set with one of the options, create it
// update the token id <-> account_id mapping
CMA_LEDGER_API int cma_ledger_retrieve_account(cma_ledger_t *ledger, cma_ledger_account_id_t *account_id,
    cma_ledger_account_t *account, const void *addr_accid, cma_ledger_account_type_t account_type,
    cma_ledger_retrieve_operation_t op);

// Deposit
CMA_LEDGER_API int cma_ledger_deposit(cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id,
    cma_ledger_account_id_t to_account_id, const cma_amount_t *deposit);

// Withdrawal
CMA_LEDGER_API int cma_ledger_withdraw(cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id,
    cma_ledger_account_id_t from_account_id, const cma_amount_t *withdrawal);

// Transfer
CMA_LEDGER_API int cma_ledger_transfer(cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id,
    cma_ledger_account_id_t from_account_id, cma_ledger_account_id_t to_account_id, const cma_amount_t *amount);

// Get balance
CMA_LEDGER_API int cma_ledger_get_balance(cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id,
    cma_ledger_account_id_t account_id, cma_amount_t *out_balance);

// Get total supply
CMA_LEDGER_API int cma_ledger_get_total_supply(cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id,
    cma_amount_t *out_total_supply);

// get error message
CMA_LEDGER_API const char *cma_ledger_get_last_error_message();

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CMA_LEDGER_H
