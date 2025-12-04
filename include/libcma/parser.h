#ifndef CMA_PARSER_H
#define CMA_PARSER_H
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Compiler visibility definition
#ifndef CMA_PARSER_API
#define CMA_PARSER_API __attribute__((visibility("default")))
#endif

#include <libcmt/abi.h>
#include <libcmt/rollup.h>

#include "types.h"

enum {
    // Bytecode for solidity WithdrawEther(uint256,bytes) = 8cf70f0b
    WITHDRAW_ETHER = 0x8cf70f0b,
    // Bytecode for solidity WithdrawErc20(address,uint256,bytes) = 4f94d342
    WITHDRAW_ERC20 = 0x4f94d342,
    // Bytecode for solidity WithdrawErc721(address,uint256,bytes) = 33acf293
    WITHDRAW_ERC721 = 0x33acf293,
    // Bytecode for solidity WithdrawErc1155Single(address,uint256,uint256,bytes) = 8bb0a811
    WITHDRAW_ERC1155_SINGLE = 0x8bb0a811,
    // Bytecode for solidity WithdrawErc1155Batch(address,uint256[],uint256[],bytes) = 50c80019
    WITHDRAW_ERC1155_BATCH = 0x50c80019,

    // Bytecode for solidity TransferEther(bytes32,uint256,bytes) = ff67c903
    TRANSFER_ETHER = 0xff67c903,
    // Bytecode for solidity TransferErc20(address,bytes32,uint256,bytes) = 03d61dcd
    TRANSFER_ERC20 = 0x03d61dcd,
    // Bytecode for solidity TransferErc721(address,bytes32,uint256,bytes) = af615a5a
    TRANSFER_ERC721 = 0xaf615a5a,
    // Bytecode for solidity TransferErc1155Single(address,bytes32,uint256,uint256,bytes) = e1c913ed
    TRANSFER_ERC1155_SINGLE = 0xe1c913ed,
    // Bytecode for solidity TransferErc1155Batch(address,bytes32,uint256[],uint256[],bytes) = 638ac6f9
    TRANSFER_ERC1155_BATCH = 0x638ac6f9,

    // Bytecode for solidity transfer(address,uint256) = a9059cbb
    ERC20_TRANSFER_FUNCTION_SELECTOR_FUNSEL = 0xa9059cbb,
    // Bytecode for solidity safeTransferFrom(address,address,uint256) = 42842e0e
    ERC721_TRANSFER_FUNCTION_SELECTOR_FUNSEL = 0x42842e0e,
    // Bytecode for solidity safeTransferFrom(address,address,uint256,uint256,bytes) = f242432a
    ERC1155_SINGLE_TRANSFER_FUNCTION_SELECTOR_FUNSEL = 0xf242432a,
    // Bytecode for solidity safeBatchTransferFrom(address,address,uint256[],uint256[],bytes) = 2eb2c2d6
    ERC1155_BATCH_TRANSFER_FUNCTION_SELECTOR_FUNSEL = 0x2eb2c2d6,
};

typedef enum {
    CMA_PARSER_INPUT_TYPE_NONE,
    CMA_PARSER_INPUT_TYPE_AUTO,
    CMA_PARSER_INPUT_TYPE_ETHER_DEPOSIT,
    CMA_PARSER_INPUT_TYPE_ERC20_DEPOSIT,
    CMA_PARSER_INPUT_TYPE_ERC721_DEPOSIT,
    CMA_PARSER_INPUT_TYPE_ERC1155_SINGLE_DEPOSIT,
    CMA_PARSER_INPUT_TYPE_ERC1155_BATCH_DEPOSIT,
    CMA_PARSER_INPUT_TYPE_ETHER_WITHDRAWAL,
    CMA_PARSER_INPUT_TYPE_ERC20_WITHDRAWAL,
    CMA_PARSER_INPUT_TYPE_ERC721_WITHDRAWAL,
    CMA_PARSER_INPUT_TYPE_ERC1155_SINGLE_WITHDRAWAL,
    CMA_PARSER_INPUT_TYPE_ERC1155_BATCH_WITHDRAWAL,
    CMA_PARSER_INPUT_TYPE_ETHER_TRANSFER,
    CMA_PARSER_INPUT_TYPE_ERC20_TRANSFER,
    CMA_PARSER_INPUT_TYPE_ERC721_TRANSFER,
    CMA_PARSER_INPUT_TYPE_ERC1155_SINGLE_TRANSFER,
    CMA_PARSER_INPUT_TYPE_ERC1155_BATCH_TRANSFER,
    CMA_PARSER_INPUT_TYPE_BALANCE,
    CMA_PARSER_INPUT_TYPE_SUPPLY,
} cma_parser_input_type_t;

typedef struct cma_parser_ether_deposit {
    cma_abi_address_t sender;
    cma_amount_t amount;
    cma_abi_bytes_t exec_layer_data;
} cma_parser_ether_deposit_t;
typedef struct cma_parser_erc20_deposit {
    cma_abi_address_t sender;
    cma_token_address_t token;
    cma_amount_t amount;
    cma_abi_bytes_t exec_layer_data;
} cma_parser_erc20_deposit_t;
typedef struct cma_parser_erc721_deposit {
    cma_abi_address_t sender;
    cma_token_address_t token;
    cma_token_id_t token_id;
    cma_abi_bytes_t exec_layer_data;
} cma_parser_erc721_deposit_t;
typedef struct cma_parser_erc1155_single_deposit {
    cma_abi_address_t sender;
    cma_token_address_t token;
    cma_token_id_t token_id;
    cma_amount_t amount;
    cma_abi_bytes_t exec_layer_data;
} cma_parser_erc1155_single_deposit_t;
typedef struct cma_parser_erc1155_batch_deposit {
    cma_abi_address_t sender;
    cma_token_address_t token;
    size_t count;
    cma_token_id_t *token_ids;
    cma_amount_t *amounts;
    cma_abi_bytes_t base_layer_data;
    cma_abi_bytes_t exec_layer_data;
} cma_parser_erc1155_batch_deposit_t;

typedef struct cma_parser_ether_withdrawal {
    cma_amount_t amount;
    cma_abi_bytes_t exec_layer_data;
} cma_parser_ether_withdrawal_t;
typedef struct cma_parser_erc20_withdrawal {
    cma_token_address_t token;
    cma_amount_t amount;
    cma_abi_bytes_t exec_layer_data;
} cma_parser_erc20_withdrawal_t;
typedef struct cma_parser_erc721_withdrawal {
    cma_token_address_t token;
    cma_token_id_t token_id;
    cma_abi_bytes_t exec_layer_data;
} cma_parser_erc721_withdrawal_t;
typedef struct cma_parser_erc1155_single_withdrawal {
    cma_token_address_t token;
    cma_token_id_t token_id;
    cma_amount_t amount;
    cma_abi_bytes_t exec_layer_data;
} cma_parser_erc1155_single_withdrawal_t;
typedef struct cma_parser_erc1155_batch_withdrawal {
    cma_token_address_t token;
    size_t count;
    cma_token_id_t *token_ids;
    cma_amount_t *amounts;
    cma_abi_bytes_t exec_layer_data;
} cma_parser_erc1155_batch_withdrawal_t;

typedef struct cma_parser_ether_transfer {
    cma_account_id_t receiver;
    cma_amount_t amount;
    cma_abi_bytes_t exec_layer_data;
} cma_parser_ether_transfer_t;
typedef struct cma_parser_erc20_transfer {
    cma_account_id_t receiver;
    cma_token_address_t token;
    cma_amount_t amount;
    cma_abi_bytes_t exec_layer_data;
} cma_parser_erc20_transfer_t;
typedef struct cma_parser_erc721_transfer {
    cma_account_id_t receiver;
    cma_token_address_t token;
    cma_token_id_t token_id;
    cma_abi_bytes_t exec_layer_data;
} cma_parser_erc721_transfer_t;
typedef struct cma_parser_erc1155_single_transfer {
    cma_account_id_t receiver;
    cma_token_address_t token;
    cma_token_id_t token_id;
    cma_amount_t amount;
    cma_abi_bytes_t exec_layer_data;
} cma_parser_erc1155_single_transfer_t;
typedef struct cma_parser_erc1155_batch_transfer {
    cma_account_id_t receiver;
    cma_token_address_t token;
    size_t count;
    cma_token_id_t *token_ids;
    cma_amount_t *amounts;
    cma_abi_bytes_t exec_layer_data;
} cma_parser_erc1155_batch_transfer_t;

typedef struct cma_parser_balance_t {
    cma_account_id_t account;
    cma_token_address_t token;
    cma_token_id_t token_id;
    cma_abi_bytes_t exec_layer_data;
} cma_parser_balance_t;
typedef struct cma_parser_supply_t {
    cma_token_address_t token;
    cma_token_id_t token_id;
    cma_abi_bytes_t exec_layer_data;
} cma_parser_supply_t;

typedef struct cma_parser_input {
    cma_parser_input_type_t type;
    union {
        cma_parser_ether_deposit_t ether_deposit;
        cma_parser_erc20_deposit_t erc20_deposit;
        cma_parser_erc721_deposit_t erc721_deposit;
        cma_parser_erc1155_single_deposit_t erc1155_single_deposit;
        cma_parser_erc1155_batch_deposit_t erc1155_batch_deposit;
        cma_parser_ether_withdrawal_t ether_withdrawal;
        cma_parser_erc20_withdrawal_t erc20_withdrawal;
        cma_parser_erc721_withdrawal_t erc721_withdrawal;
        cma_parser_erc1155_single_withdrawal_t erc1155_single_withdrawal;
        cma_parser_erc1155_batch_withdrawal_t erc1155_batch_withdrawal;
        cma_parser_ether_transfer_t ether_transfer;
        cma_parser_erc20_transfer_t erc20_transfer;
        cma_parser_erc721_transfer_t erc721_transfer;
        cma_parser_erc1155_single_transfer_t erc1155_single_transfer;
        cma_parser_erc1155_batch_transfer_t erc1155_batch_transfer;
        cma_parser_balance_t balance;
        cma_parser_supply_t supply;
    };
} cma_parser_input_t;

typedef enum {
    CMA_PARSER_VOUCHER_TYPE_NONE,
    CMA_PARSER_VOUCHER_TYPE_ETHER,
    CMA_PARSER_VOUCHER_TYPE_ERC20,
    CMA_PARSER_VOUCHER_TYPE_ERC721,
    CMA_PARSER_VOUCHER_TYPE_ERC1155_SINGLE,
    CMA_PARSER_VOUCHER_TYPE_ERC1155_BATCH,
} cma_parser_voucher_type_t;

enum {
    CMA_PARSER_SELECTOR_SIZE = 4,
    CMA_PARSER_ETHER_VOUCHER_PAYLOAD_SIZE = 0,
    CMA_PARSER_ERC20_VOUCHER_PAYLOAD_SIZE = CMA_PARSER_SELECTOR_SIZE + 2 * CMA_ABI_U256_LENGTH,
    CMA_PARSER_ERC721_VOUCHER_PAYLOAD_SIZE = CMA_PARSER_SELECTOR_SIZE + 3 * CMA_ABI_U256_LENGTH,
    CMA_PARSER_ERC1155_SINGLE_VOUCHER_PAYLOAD_MIN_SIZE = CMA_PARSER_SELECTOR_SIZE + 6 * CMA_ABI_U256_LENGTH,
    CMA_PARSER_ERC1155_BATCH_VOUCHER_PAYLOAD_MIN_SIZE = CMA_PARSER_SELECTOR_SIZE + 8 * CMA_ABI_U256_LENGTH,
};

typedef struct cma_parser_ether_voucher_fields {
    cma_amount_t amount;
} cma_parser_ether_voucher_fields_t;
typedef struct cma_parser_erc20_voucher_fields {
    cma_token_address_t token;
    cma_amount_t amount;
} cma_parser_erc20_voucher_fields_t;
typedef struct cma_parser_erc721_voucher_fields {
    cma_token_address_t token;
    cma_token_id_t token_id;
    cma_abi_bytes_t exec_layer_data;
} cma_parser_erc721_voucher_fields_t;
typedef struct cma_parser_erc1155_single_voucher_fields {
    cma_token_address_t token;
    cma_token_id_t token_id;
    cma_amount_t amount;
} cma_parser_erc1155_single_voucher_fields_t;
typedef struct cma_parser_erc1155_batch_voucher_fields {
    cma_token_address_t token;
    size_t count;
    cma_token_id_t *token_ids;
    cma_amount_t *amounts;
} cma_parser_erc1155_batch_voucher_fields_t;

typedef struct cma_parser_voucher_data {
    cma_abi_address_t receiver;
    union {
        cma_parser_ether_voucher_fields_t ether_voucher_fields;
        cma_parser_erc20_voucher_fields_t erc20_voucher_fields;
        cma_parser_erc721_voucher_fields_t erc721_voucher_fields;
        cma_parser_erc1155_single_voucher_fields_t erc1155_single_voucher_fields;
        cma_parser_erc1155_batch_voucher_fields_t erc1155_batch_voucher_fields;
    };
} cma_parser_voucher_data_t;

typedef struct cma_voucher {
    cmt_abi_address_t address;
    cmt_abi_u256_t value;
    cmt_abi_bytes_t payload;
} cma_voucher_t;

enum {
    CMA_PARSER_SUCCESS = 0,
    CMA_PARSER_ERROR_UNKNOWN = -2001,
    CMA_PARSER_ERROR_EXCEPTION = -2002,
    CMA_PARSER_ERROR_INCOMPATIBLE_INPUT = -2003,
    CMA_PARSER_ERROR_MALFORMED_INPUT = -2004,
    CMA_PARSER_ERROR_INVALID_AMOUNT = -2005,
};

// decode advance
CMA_PARSER_API int cma_parser_decode_advance(cma_parser_input_type_t type, const cmt_rollup_advance_t *input,
    cma_parser_input_t *parser_input);

// decode inspect
CMA_PARSER_API int cma_parser_decode_inspect(cma_parser_input_type_t type, const cmt_rollup_inspect_t *input,
    cma_parser_input_t *parser_input);

// encode voucher
CMA_PARSER_API int cma_parser_encode_voucher(cma_parser_voucher_type_t type, const cma_abi_address_t *app_address,
    const cma_parser_voucher_data_t *voucher_request, cma_voucher_t *voucher);

// get error message
CMA_PARSER_API const char *cma_parser_get_last_error_message();

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CMA_PARSER_H
