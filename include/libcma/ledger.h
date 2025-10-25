#ifndef CMA_LEDGER_H
#define CMA_LEDGER_H
#include <stdint.h>

#include <libcmt/abi.h>

enum {
  CMA_ABI_BYTES32_LENGTH = 32,
};

typedef struct cma_abi_account {
  uint8_t data[CMA_ABI_BYTES32_LENGTH];
} cma_abi_account_t;

typedef cmt_abi_u256 amount_t;
typedef cmt_abi_u256 token_id_t;
typedef cmt_abi_address token_address_t;
typedef cma_abi_account_t account_t;
typedef uint64_t wallet_t;
typedef uint64_t asset_t;

typedef enum {
  CMA_LEDGER_SUCCESS = 0,
  CMA_LEDGER_ERROR_INSUFFICIENT_FUNDS,
  CMA_LEDGER_ERROR_WALLET_NOT_FOUND,
  CMA_LEDGER_ERROR_ASSET_NOT_FOUND,
  CMA_LEDGER_ERROR_WALLETS_LIMIT,
  CMA_LEDGER_ERROR_ASSETS_LIMIT,
  CMA_LEDGER_ERROR_UNKNOWN
} cma_ledger_error_t;

cma_ledger_error_t ledger_init(void);
cma_ledger_error_t ledger_asset_create(
    token_address_t *token_address, token_id_t token_id,
    asset_t *out_asset_id); // Create a new asset for the given token and
                            // update the token (address, token id) -> asset_id
                            // mapping

cma_ledger_error_t ledger_wallet_create(
    account_t *account_id,
    wallet_t *out_wallet_id); // Create a new wallet for the given account and
                              // update the account -> wallet_id mapping

cma_ledger_error_t
ledger_asset_retrieve(token_address_t *token_address, token_id_t token_id,
                      asset_t *asset_id); // Retrieve an existing asset id for
                                          // the given token (address, token id)

cma_ledger_error_t ledger_wallet_retrieve(
    account_t *account_id,
    wallet_t
        *wallet_id); // Retrieve an existing wallet id for the given account

cma_ledger_error_t
ledger_token_retrieve(token_address_t *token_address, token_id_t token_id,
                      asset_t asset_id); // Retrieve the token (address, token
                                         // id) given the asset id

cma_ledger_error_t ledger_wallet_retrieve(
    account_t *account_id,
    wallet_t wallet_id); // Retrieve the account given the wallet id

bool ledger_wallet_exists(wallet_t wallet_id);
bool ledger_asset_exists(asset_t asset_id);

cma_ledger_error_t ledger_deposit(asset_t asset_id, wallet_t to_wallet_id,
                                  amount_t *amount);

cma_ledger_error_t ledger_withdrawal(asset_t asset_id, wallet_t from_wallet_id,
                                     amount_t *amount);

cma_ledger_error_t ledger_transfer(asset_t asset_id, wallet_t from_wallet_id,
                                   wallet_t to_wallet_id, amount_t *amount);

cma_ledger_error_t ledger_get_balance(asset_t asset_id, wallet_t wallet_id,
                                      amount_t *out_balance);

cma_ledger_error_t ledger_get_total_supply(asset_t asset_id,
                                           amount_t *out_total_supply);

#endif // CMA_LEDGER_H
