#ifndef CMA_ABI_H
#define CMA_ABI_H

#include <stdint.h>

#include <libcmt/abi.h>

enum {
  CMA_ABI_BYTES32_LENGTH = 32,
};

// typedef struct wallet_address {
//   uint8_t fix[CMA_ABI_BYTES32_LENGTH - CMT_ABI_ADDRESS_LENGTH];
//   uint8_t data[CMT_ABI_ADDRESS_LENGTH];
// } wallet_address_t;

typedef struct cma_bytes32 {
  uint8_t data[CMA_ABI_BYTES32_LENGTH];
} cma_bytes32_t;

typedef cmt_abi_address_t cma_abi_address_t;
typedef cmt_abi_bytes_t cma_abi_bytes_t;
typedef cmt_abi_u256_t cma_amount_t;
typedef cmt_abi_u256_t cma_token_id_t;
typedef cmt_abi_address_t cma_token_address_t;
typedef cma_bytes32_t cma_account_t;

#endif // CMA_ABI_H
