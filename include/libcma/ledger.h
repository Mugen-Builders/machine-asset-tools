#ifndef CMA_LEDGER_H
#define CMA_LEDGER_H
#include <stdint.h>

#include "types.h"

typedef uint64_t cma_wallet_t;
typedef uint64_t cma_asset_t;

typedef enum {
  CMA_LEDGER_SUCCESS = 0,
  CMA_LEDGER_ERROR_INSUFFICIENT_FUNDS = 1001,
  CMA_LEDGER_ERROR_WALLET_NOT_FOUND = 1002,
  CMA_LEDGER_ERROR_ASSET_NOT_FOUND = 1003,
  CMA_LEDGER_ERROR_WALLETS_LIMIT = 1004,
  CMA_LEDGER_ERROR_ASSETS_LIMIT = 1005,
  CMA_LEDGER_ERROR_INVALID_WALLET = 1006,
  CMA_LEDGER_ERROR_UNKNOWN = 1007
} cma_ledger_error_t;

cma_ledger_error_t ledger_init(void);
cma_ledger_error_t ledger_asset_create(
    const cma_token_address_t *token_address, const cma_token_id_t *token_id,
    cma_asset_t *out_asset_id); // Create a new asset for the given token and
                                // update the token (address, token id) ->
                                // asset_id mapping

cma_ledger_error_t ledger_wallet_create(
    const cma_account_t *account_id,
    cma_wallet_t *out_wallet_id); // Create a new wallet for the given account
                                  // and update the account -> wallet_id mapping

cma_ledger_error_t ledger_wallet_address_create(
    const cma_abi_address_t *wallet_address,
    cma_wallet_t *out_wallet_id); // Create a new wallet for the given addres
                                  // and update the account -> wallet_id mapping

cma_ledger_error_t ledger_asset_retrieve(
    const cma_token_address_t *token_address, const cma_token_id_t *token_id,
    cma_asset_t *asset_id); // Retrieve an existing asset id for
                            // the given token (address, token id)

cma_ledger_error_t ledger_wallet_retrieve(
    const cma_account_t *account_id,
    cma_wallet_t
        *wallet_id); // Retrieve an existing wallet id for the given account

cma_ledger_error_t ledger_wallet_address_retrieve(
    const cma_abi_address_t *wallet_address,
    cma_wallet_t *wallet_id); // Retrieve an existing wallet id for the given
                              // wallet address

cma_ledger_error_t
ledger_token_retrieve(cma_asset_t asset_id, cma_token_address_t *token_address,
                      cma_token_id_t *token_id); // Retrieve the token (address,
                                                 // token id) given the asset id

cma_ledger_error_t ledger_account_retrieve(
    cma_wallet_t wallet_id,
    cma_account_t *account_id); // Retrieve the account given the wallet id

cma_ledger_error_t ledger_account_address_retrieve(
    cma_wallet_t wallet_id,
    cma_abi_address_t
        *wallet_address); // Retrieve the wallet address given the wallet id

bool ledger_wallet_exists(cma_wallet_t wallet_id);
bool ledger_asset_exists(cma_asset_t asset_id);

cma_ledger_error_t ledger_deposit(cma_asset_t asset_id,
                                  cma_wallet_t to_wallet_id,
                                  const cma_amount_t *amount);

cma_ledger_error_t ledger_withdrawal(cma_asset_t asset_id,
                                     cma_wallet_t from_wallet_id,
                                     const cma_amount_t *amount);

cma_ledger_error_t ledger_transfer(cma_asset_t asset_id,
                                   cma_wallet_t from_wallet_id,
                                   cma_wallet_t to_wallet_id,
                                   const cma_amount_t *amount);

cma_ledger_error_t ledger_get_balance(cma_asset_t asset_id,
                                      cma_wallet_t wallet_id,
                                      cma_amount_t *out_balance);

cma_ledger_error_t ledger_get_total_supply(cma_asset_t asset_id,
                                           cma_amount_t *out_total_supply);

#endif // CMA_LEDGER_H
