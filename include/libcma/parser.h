#ifndef CMA_PARSER_H
#define CMA_PARSER_H
#include <stddef.h>
#include <stdint.h>

#include <libcmt/abi.h>
#include <libcmt/rollup.h>

#include "types.h"

// Bytecode for solidity WithdrawEther(uint256,bytes) = 8cf70f0b
#define WITHDRAW_ETHER CMT_ABI_FUNSEL(0x8c, 0xf7, 0x0f, 0x0b)

// Bytecode for solidity WithdrawErc20(address,uint256,bytes) = 4f94d342
#define WITHDRAW_ERC20 CMT_ABI_FUNSEL(0x4f, 0x94, 0xd3, 0x42)

// Bytecode for solidity WithdrawErc721(address,uint256,bytes) = 33acf293
#define WITHDRAW_ERC721 CMT_ABI_FUNSEL(0x33, 0xac, 0xf2, 0x93)

// Bytecode for solidity WithdrawErc1155Single(address,uint256,uint256,bytes) =
// 8bb0a811
#define WITHDRAW_ERC1155_SINGLE CMT_ABI_FUNSEL(0x8b, 0xb0, 0xa8, 0x11)

// Bytecode for solidity WithdrawErc1155Batch(address,uint256[],uint256[],bytes)
// = 50c80019
#define WITHDRAW_ERC1155_BATCH CMT_ABI_FUNSEL(0x50, 0xc8, 0x00, 0x19)

// Bytecode for solidity TransferEther(uint256,bytes32,bytes) = 428c9c4d
#define TRANSFER_ETHER CMT_ABI_FUNSEL(0x42, 0x8c, 0x9c, 0x4d)

// Bytecode for solidity TransferErc20(address,bytes32,uint256,bytes) = 03d61dcd
#define TRANSFER_ERC20 CMT_ABI_FUNSEL(0x03, 0xd6, 0x1d, 0xcd)

// Bytecode for solidity TransferErc721(address,bytes32,uint256,bytes) =
// af615a5a
#define TRANSFER_ERC721 CMT_ABI_FUNSEL(0xaf, 0x61, 0x5a, 0x5a)

// Bytecode for solidity
// TransferErc1155Single(address,bytes32,uint256,uint256,bytes) = e1c913ed
#define TRANSFER_ERC1155_SINGLE CMT_ABI_FUNSEL(0xe1, 0xc9, 0x13, 0xed)

// Bytecode for solidity
// TransferErc1155Batch(address,bytes32,uint256[],uint256[],bytes) = 638ac6f9
#define TRANSFER_ERC1155_BATCH CMT_ABI_FUNSEL(0x63, 0x8a, 0xc6, 0xf9)

// Bytecode for solidity transfer(address,uint256) = a9059cbb
#define ERC20_TRANSFER_FUNCTION_SELECTOR_FUNSEL                                \
  CMT_ABI_FUNSEL(0xa9, 0x05, 0x9c, 0xbb)

// Bytecode for solidity safeTransferFrom(address,address,uint256) = 42842e0e
#define ERC721_TRANSFER_FUNCTION_SELECTOR_FUNSEL                               \
  CMT_ABI_FUNSEL(0x42, 0x84, 0x2e, 0x0e)

// Bytecode for solidity
// safeTransferFrom(address,address,uint256,uint256,bytes) = f242432a
#define ERC1155_SINGLE_TRANSFER_FUNCTION_SELECTOR_FUNSEL                       \
  CMT_ABI_FUNSEL(0xf2, 0x42, 0x43, 0x2a)

// Bytecode for solidity
// safeBatchTransferFrom(address,address,uint256[],uint256[],bytes) = 2eb2c2d6
#define ERC1155_BATCH_TRANSFER_FUNCTION_SELECTOR_FUNSEL                        \
  CMT_ABI_FUNSEL(0x2e, 0xb2, 0xc2, 0xd6)

enum cma_parser_request_type_t {
  CMA_PARSER_REQUEST_TYPE_NONE,
  CMA_PARSER_REQUEST_TYPE_ETHER_DEPOSIT,
  CMA_PARSER_REQUEST_TYPE_ERC20_DEPOSIT,
  CMA_PARSER_REQUEST_TYPE_ERC721_DEPOSIT,
  CMA_PARSER_REQUEST_TYPE_ERC1155_SINGLE_DEPOSIT,
  CMA_PARSER_REQUEST_TYPE_ERC1155_BATCH_DEPOSIT,
  CMA_PARSER_REQUEST_TYPE_ETHER_WITHDRAWAL,
  CMA_PARSER_REQUEST_TYPE_ERC20_WITHDRAWAL,
  CMA_PARSER_REQUEST_TYPE_ERC721_WITHDRAWAL,
  CMA_PARSER_REQUEST_TYPE_ERC1155_SINGLE_WITHDRAWAL,
  CMA_PARSER_REQUEST_TYPE_ERC1155_BATCH_WITHDRAWAL,
  CMA_PARSER_REQUEST_TYPE_ETHER_TRANSFER,
  CMA_PARSER_REQUEST_TYPE_ERC20_TRANSFER,
  CMA_PARSER_REQUEST_TYPE_ERC721_TRANSFER,
  CMA_PARSER_REQUEST_TYPE_ERC1155_SINGLE_TRANSFER,
  CMA_PARSER_REQUEST_TYPE_ERC1155_BATCH_TRANSFER,
  CMA_PARSER_REQUEST_TYPE_BALANCE,
  CMA_PARSER_REQUEST_TYPE_SUPPLY,
};

struct cma_parser_ether_deposit_t {
  cma_abi_address_t sender;
  cma_amount_t amount;
  cma_abi_bytes_t exec_layer_data;
};

struct cma_parser_erc20_deposit_t {
  cma_abi_address_t sender;
  cma_token_address_t token;
  cma_amount_t amount;
  cma_abi_bytes_t exec_layer_data;
};

struct cma_parser_erc721_deposit_t {
  cma_abi_address_t sender;
  cma_token_address_t token;
  cma_token_id_t token_id;
  cma_abi_bytes_t exec_layer_data;
};

struct cma_parser_erc1155_single_deposit_t {
  cma_abi_address_t sender;
  cma_token_address_t token;
  cma_token_id_t token_id;
  cma_amount_t amount;
  cma_abi_bytes_t exec_layer_data;
};

struct cma_parser_erc1155_batch_deposit_t {
  cma_abi_address_t sender;
  cma_token_address_t token;
  size_t count;
  cma_token_id_t *token_ids;
  cma_amount_t *amounts;
  cma_abi_bytes_t base_layer_data;
  cma_abi_bytes_t exec_layer_data;
};

struct cma_parser_ether_withdrawal_t {
  cma_amount_t amount;
  cma_abi_bytes_t exec_layer_data;
};

struct cma_parser_erc20_withdrawal_t {
  cma_token_address_t token;
  cma_amount_t amount;
  cma_abi_bytes_t exec_layer_data;
};
struct cma_parser_erc721_withdrawal_t {
  cma_token_address_t token;
  cma_token_id_t token_id;
  cma_abi_bytes_t exec_layer_data;
};
struct cma_parser_erc1155_single_withdrawal_t {
  cma_token_address_t token;
  cma_token_id_t token_id;
  cma_amount_t amount;
  cma_abi_bytes_t exec_layer_data;
};
struct cma_parser_erc1155_batch_withdrawal_t {
  cma_token_address_t token;
  size_t count;
  cma_token_id_t *token_ids;
  cma_amount_t *amounts;
  cma_abi_bytes_t exec_layer_data;
};

struct cma_parser_ether_transfer_t {
  cma_account_t receiver;
  cma_amount_t amount;
  cma_abi_bytes_t exec_layer_data;
};
struct cma_parser_erc20_transfer_t {
  cma_account_t receiver;
  cma_token_address_t token;
  cma_amount_t amount;
  cma_abi_bytes_t exec_layer_data;
};
struct cma_parser_erc721_transfer_t {
  cma_account_t receiver;
  cma_token_address_t token;
  cma_token_id_t token_id;
  cma_abi_bytes_t exec_layer_data;
};
struct cma_parser_erc1155_single_transfer_t {
  cma_account_t receiver;
  cma_token_address_t token;
  cma_token_id_t token_id;
  cma_amount_t amount;
  cma_abi_bytes_t exec_layer_data;
};
struct cma_parser_erc1155_batch_transfer_t {
  cma_account_t receiver;
  cma_token_address_t token;
  size_t count;
  cma_token_id_t *token_ids;
  cma_amount_t *amounts;
  cma_abi_bytes_t exec_layer_data;
};

struct cma_parser_balance_t {
  cma_account_t account;
  cma_token_address_t token;
  cma_token_id_t token_id;
  cma_abi_bytes_t exec_layer_data;
};
struct cma_parser_supply_t {
  cma_token_address_t token;
  cma_token_id_t token_id;
  cma_abi_bytes_t exec_layer_data;
};

typedef struct cma_parser_request {
  enum cma_parser_request_type_t type;
  union {
    struct cma_parser_ether_deposit_t ether_deposit;
    struct cma_parser_erc20_deposit_t erc20_deposit;
    struct cma_parser_erc721_deposit_t erc721_deposit;
    struct cma_parser_erc1155_single_deposit_t erc1155_single_deposit;
    struct cma_parser_erc1155_batch_deposit_t erc1155_batch_deposit;
    struct cma_parser_ether_withdrawal_t ether_withdrawal;
    struct cma_parser_erc20_withdrawal_t erc20_withdrawal;
    struct cma_parser_erc721_withdrawal_t erc721_withdrawal;
    struct cma_parser_erc1155_single_withdrawal_t erc1155_single_withdrawal;
    struct cma_parser_erc1155_batch_withdrawal_t erc1155_batch_withdrawal;
    struct cma_parser_ether_transfer_t ether_transfer;
    struct cma_parser_erc20_transfer_t erc20_transfer;
    struct cma_parser_erc721_transfer_t erc721_transfer;
    struct cma_parser_erc1155_single_transfer_t erc1155_single_transfer;
    struct cma_parser_erc1155_batch_transfer_t erc1155_batch_transfer;
    struct cma_parser_balance_t balance;
    struct cma_parser_supply_t supply;
  } u;
} cma_parser_request_t;

typedef struct cma_voucher {
  cmt_abi_address_t address;
  cmt_abi_u256_t value;
  cmt_abi_bytes_t data;
} cma_voucher_t;

typedef enum {
  CMA_PARSER_SUCCESS = 0,
  CMA_PARSER_ERROR_UNKNOWN = 2001
} cma_parser_error_t;

// decode request
// Note: app's responsability to identify deposits (returns error if it is not
// withdrawal or  transfer)
cma_parser_error_t cma_parser_rollup(const cmt_rollup_t *rollup,
                                     cma_parser_request_t *request);

// decode ether deposit request
cma_parser_error_t cma_decode_ether_deposit(const cmt_rollup_advance_t *input,
                                            cma_parser_request_t *request);
// decode erc20 deposit request
cma_parser_error_t cma_decode_erc20_deposit(const cmt_rollup_advance_t *input,
                                            cma_parser_request_t *request);
// decode erc721 deposit request
cma_parser_error_t cma_decode_erc721_deposit(const cmt_rollup_advance_t *input,
                                             cma_parser_request_t *request);
// decode erc1155 single deposit request
cma_parser_error_t
cma_decode_erc1155_single_deposit(const cmt_rollup_advance_t *input,
                                  cma_parser_request_t *request);
// decode erc1155 batch deposit request
cma_parser_error_t
cma_decode_erc1155_batch_deposit(const cmt_rollup_advance_t *input,
                                 cma_parser_request_t *request);

// decode ether withdrawal request
cma_parser_error_t
cma_decode_ether_withdrawal(const cmt_rollup_advance_t *input,
                            cma_parser_request_t *request);
// decode erc20 withdrawal request
cma_parser_error_t
cma_decode_erc20_withdrawal(const cmt_rollup_advance_t *input,
                            cma_parser_request_t *request);
// decode erc721 withdrawal request
cma_parser_error_t
cma_decode_erc721_withdrawal(const cmt_rollup_advance_t *input,
                             cma_parser_request_t *request);
// decode erc1155 single withdrawal request
cma_parser_error_t
cma_decode_erc1155_single_withdrawal(const cmt_rollup_advance_t *input,
                                     cma_parser_request_t *request);
// decode erc1155 batch withdrawal request
cma_parser_error_t
cma_decode_erc1155_batch_withdrawal(const cmt_rollup_advance_t *input,
                                    cma_parser_request_t *request);

// decode ether transfer request
cma_parser_error_t cma_decode_ether_transfer(const cmt_rollup_advance_t *input,
                                             cma_parser_request_t *request);
// decode erc20 transfer request
cma_parser_error_t cma_decode_erc20_transfer(const cmt_rollup_advance_t *input,
                                             cma_parser_request_t *request);
// decode erc721 transfer request
cma_parser_error_t cma_decode_erc721_transfer(const cmt_rollup_advance_t *input,
                                              cma_parser_request_t *request);
// decode erc1155 single transfer request
cma_parser_error_t
cma_decode_erc1155_single_transfer(const cmt_rollup_advance_t *input,
                                   cma_parser_request_t *request);
// decode erc1155 batch transfer request
cma_parser_error_t
cma_decode_erc1155_batch_transfer(const cmt_rollup_advance_t *input,
                                  cma_parser_request_t *request);

// decode balance request
cma_parser_error_t cma_decode_balance(const cmt_rollup_inspect_t *input,
                                      cma_parser_request_t *request);

// decode supply request
cma_parser_error_t cma_decode_supply(const cmt_rollup_inspect_t *input,
                                     cma_parser_request_t *request);

// encode ether withdrawal voucher
cma_parser_error_t
cma_encode_ether_withdrawal(const cma_abi_address_t *wallet_address,
                            const cma_amount_t *amount, cma_voucher_t *voucher);

// encode erc20 withdrawal voucher
cma_parser_error_t
cma_encode_erc20_withdrawal(const cma_abi_address_t *wallet_address,
                            const cma_amount_t *amount, cma_voucher_t *voucher);

// encode erc721 withdrawal voucher
cma_parser_error_t
cma_encode_erc721_withdrawal(const cma_abi_address_t *wallet_address,
                             const cma_token_id_t *token_id,
                             cma_voucher_t *voucher);

// encode erc1155 single withdrawal voucher
cma_parser_error_t
cma_encode_erc1155_single_withdrawal(const cma_abi_address_t *wallet_address,
                                     const cma_amount_t *amount,
                                     const cma_token_id_t *token_id,
                                     cma_voucher_t *voucher);

// encode erc1155 batch withdrawal voucher
cma_parser_error_t
cma_encode_erc1155_batch_withdrawal(const cma_abi_address_t *wallet_address,
                                    size_t count,
                                    const cma_amount_t *amounts,
                                    const cma_token_id_t *token_ids,
                                    cma_voucher_t *voucher);

#endif // CMA_PARSER_H
