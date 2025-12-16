from libc.stdint cimport uint64_t, uint8_t
from libcmt cimport cmt_abi_address_t, cmt_abi_bytes_t, cmt_abi_u256_t, \
    cmt_rollup_advance_t, cmt_rollup_inspect_t, CMT_ABI_ADDRESS_LENGTH, CMT_ABI_U256_LENGTH

cdef extern from "/usr/include/libcma/types.h":
    ctypedef cmt_abi_address_t cma_abi_address_t
    ctypedef cmt_abi_bytes_t cma_abi_bytes_t
    ctypedef cmt_abi_u256_t cma_amount_t
    ctypedef cmt_abi_u256_t cma_token_id_t
    ctypedef cmt_abi_u256_t cma_bytes32_t
    ctypedef cmt_abi_address_t cma_token_address_t
    ctypedef cma_bytes32_t cma_account_id_t

cdef extern from "/usr/include/libcma/parser.h":
    ctypedef enum cma_parser_input_type_t:
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
        CMA_PARSER_INPUT_TYPE_BALANCE_ACCOUNT,
        CMA_PARSER_INPUT_TYPE_BALANCE_ACCOUNT_TOKEN_ADDRESS,
        CMA_PARSER_INPUT_TYPE_BALANCE_ACCOUNT_TOKEN_ADDRESS_ID,
        CMA_PARSER_INPUT_TYPE_SUPPLY,
        CMA_PARSER_INPUT_TYPE_SUPPLY_TOKEN_ADDRESS,
        CMA_PARSER_INPUT_TYPE_SUPPLY_TOKEN_ADDRESS_ID,

    ctypedef enum cma_parser_voucher_type_t:
        CMA_PARSER_VOUCHER_TYPE_NONE,
        CMA_PARSER_VOUCHER_TYPE_ETHER,
        CMA_PARSER_VOUCHER_TYPE_ERC20,
        CMA_PARSER_VOUCHER_TYPE_ERC721,
        CMA_PARSER_VOUCHER_TYPE_ERC1155_SINGLE,
        CMA_PARSER_VOUCHER_TYPE_ERC1155_BATCH,

    ctypedef enum:
        CMA_PARSER_SELECTOR_SIZE,
        CMA_PARSER_ETHER_VOUCHER_PAYLOAD_SIZE,
        CMA_PARSER_ERC20_VOUCHER_PAYLOAD_SIZE,
        CMA_PARSER_ERC721_VOUCHER_PAYLOAD_SIZE,
        CMA_PARSER_ERC1155_SINGLE_VOUCHER_PAYLOAD_MIN_SIZE,
        CMA_PARSER_ERC1155_BATCH_VOUCHER_PAYLOAD_MIN_SIZE,

    ctypedef struct cma_parser_ether_deposit_t:
        cma_abi_address_t sender
        cma_amount_t amount
        cma_abi_bytes_t exec_layer_data
    ctypedef struct cma_parser_erc20_deposit_t:
        cma_abi_address_t sender
        cma_token_address_t token
        cma_amount_t amount
        cma_abi_bytes_t exec_layer_data
    # ctypedef struct cma_parser_erc721_deposit_t:
    #     cma_abi_address_t sender
    #     cma_token_address_t token
    #     cma_token_id_t token_id
    #     cma_abi_bytes_t exec_layer_data
    # ctypedef struct cma_parser_erc1155_single_deposit_t:
    #     cma_abi_address_t sender
    #     cma_token_address_t token
    #     cma_token_id_t token_id
    #     cma_amount_t amount
    #     cma_abi_bytes_t exec_layer_data
    # ctypedef struct cma_parser_erc1155_batch_deposit_t:
    #     cma_abi_address_t sender
    #     cma_token_address_t token
    #     size_t count
    #     cma_token_id_t *token_ids
    #     cma_amount_t *amounts
    #     cma_abi_bytes_t base_layer_data
    #     cma_abi_bytes_t exec_layer_data
    ctypedef struct cma_parser_ether_withdrawal_t:
        cma_amount_t amount
        cma_abi_bytes_t exec_layer_data
    ctypedef struct cma_parser_erc20_withdrawal_t:
        cma_token_address_t token
        cma_amount_t amount
        cma_abi_bytes_t exec_layer_data
    # ctypedef struct cma_parser_erc721_withdrawal_t:
    #     cma_token_address_t token
    #     cma_token_id_t token_id
    #     cma_abi_bytes_t exec_layer_data
    # ctypedef struct cma_parser_erc1155_single_withdrawal_t:
    #     cma_token_address_t token
    #     cma_token_id_t token_id
    #     cma_amount_t amount
    #     cma_abi_bytes_t exec_layer_data
    # ctypedef struct cma_parser_erc1155_batch_withdrawal_t:
    #     cma_token_address_t token
    #     size_t count
    #     cma_token_id_t *token_ids
    #     cma_amount_t *amounts
    #     cma_abi_bytes_t exec_layer_data
    ctypedef struct cma_parser_ether_transfer_t:
        cma_account_id_t receiver
        cma_amount_t amount
        cma_abi_bytes_t exec_layer_data
    ctypedef struct cma_parser_erc20_transfer_t:
        cma_account_id_t receiver
        cma_token_address_t token
        cma_amount_t amount
        cma_abi_bytes_t exec_layer_data
    # ctypedef struct cma_parser_erc721_transfer_t:
    #     cma_account_id_t receiver
    #     cma_token_address_t token
    #     cma_token_id_t token_id
    #     cma_abi_bytes_t exec_layer_data
    # ctypedef struct cma_parser_erc1155_single_transfer_t:
    #     cma_account_id_t receiver
    #     cma_token_address_t token
    #     cma_token_id_t token_id
    #     cma_amount_t amount
    #     cma_abi_bytes_t exec_layer_data
    # ctypedef struct cma_parser_erc1155_batch_transfer_t:
    #     cma_account_id_t receiver
    #     cma_token_address_t token
    #     size_t count
    #     cma_token_id_t *token_ids
    #     cma_amount_t *amounts
    #     cma_abi_bytes_t exec_layer_data
    ctypedef struct cma_parser_balance_t:
        cma_account_id_t account
        cma_token_address_t token
        cma_token_id_t token_id
        cma_abi_bytes_t exec_layer_data
    ctypedef struct cma_parser_supply_t:
        cma_token_address_t token
        cma_token_id_t token_id
        cma_abi_bytes_t exec_layer_data
    ctypedef struct cma_parser_input_t:
        cma_parser_input_type_t type
        cma_parser_ether_deposit_t ether_deposit
        cma_parser_erc20_deposit_t erc20_deposit
        # cma_parser_erc721_deposit_t erc721_deposit
        # cma_parser_erc1155_single_deposit_t erc1155_single_deposit
        # cma_parser_erc1155_batch_deposit_t erc1155_batch_deposit
        cma_parser_ether_withdrawal_t ether_withdrawal
        cma_parser_erc20_withdrawal_t erc20_withdrawal
        # cma_parser_erc721_withdrawal_t erc721_withdrawal
        # cma_parser_erc1155_single_withdrawal_t erc1155_single_withdrawal
        # cma_parser_erc1155_batch_withdrawal_t erc1155_batch_withdrawal
        cma_parser_ether_transfer_t ether_transfer
        cma_parser_erc20_transfer_t erc20_transfer
        # cma_parser_erc721_transfer_t erc721_transfer
        # cma_parser_erc1155_single_transfer_t erc1155_single_transfer
        # cma_parser_erc1155_batch_transfer_t erc1155_batch_transfer
        cma_parser_balance_t balance
        cma_parser_supply_t supply

    ctypedef struct cma_parser_ether_voucher_fields_t:
        cma_amount_t amount
    ctypedef struct cma_parser_erc20_voucher_fields_t:
        cma_token_address_t token
        cma_amount_t amount
    # ctypedef struct cma_parser_erc721_voucher_fields_t:
    #     cma_token_address_t token
    #     cma_token_id_t token_id
    #     cma_abi_bytes_t exec_layer_data
    # ctypedef struct cma_parser_erc1155_single_voucher_fields_t:
    #     cma_token_address_t token
    #     cma_token_id_t token_id
    #     cma_amount_t amount
    # ctypedef struct cma_parser_erc1155_batch_voucher_fields_t:
    #     cma_token_address_t token
    #     size_t count
    #     cma_token_id_t *token_ids
    #     cma_amount_t *amounts
    ctypedef struct cma_parser_voucher_data_t:
        cma_abi_address_t receiver
        cma_parser_ether_voucher_fields_t ether
        cma_parser_erc20_voucher_fields_t erc20
        # cma_parser_erc721_voucher_fields_t erc721
        # cma_parser_erc1155_single_voucher_fields_t erc1155_single
        # cma_parser_erc1155_batch_voucher_fields_t erc1155_batch
    ctypedef struct cma_voucher_t:
        cmt_abi_address_t address
        cmt_abi_u256_t value
        cmt_abi_bytes_t payload

    # decode advance
    int cma_parser_decode_advance(cma_parser_input_type_t type, cmt_rollup_advance_t *input,
        cma_parser_input_t *parser_input)
    # decode inspect
    int cma_parser_decode_inspect(cma_parser_input_type_t type, cmt_rollup_inspect_t *input,
        cma_parser_input_t *parser_input)
    # encode voucher
    int cma_parser_encode_voucher(cma_parser_voucher_type_t type, cma_abi_address_t *app_address,
        cma_parser_voucher_data_t *voucher_request, cma_voucher_t *voucher)
    # get error message
    char *cma_parser_get_last_error_message()

cdef extern from "/usr/include/libcma/ledger.h":
    ctypedef uint64_t cma_ledger_account_id_t
    ctypedef uint64_t cma_ledger_asset_id_t
    ctypedef enum cma_ledger_retrieve_operation_t:
        CMA_LEDGER_OP_FIND,
        CMA_LEDGER_OP_CREATE,
        CMA_LEDGER_OP_FIND_OR_CREATE,
    ctypedef enum cma_ledger_asset_type_t:
        CMA_LEDGER_ASSET_TYPE_ID,
        CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS,
        CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID,
    ctypedef enum cma_ledger_account_type_t:
        CMA_LEDGER_ACCOUNT_TYPE_ID,
        CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS,
        CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID,

    ctypedef struct cma_ledger_t:
        pass

    ctypedef struct cma_ledger_account_t:
        cma_ledger_account_type_t type
        cma_account_id_t account_id
        cma_abi_address_t address

    int cma_ledger_init(cma_ledger_t *ledger)
    int cma_ledger_fini(cma_ledger_t *ledger)
    int cma_ledger_reset(cma_ledger_t *ledger)
    # Retrieve/create an asset
    int cma_ledger_retrieve_asset(cma_ledger_t *ledger, cma_ledger_asset_id_t *asset_id,
        cma_token_address_t *token_address, cma_token_id_t *token_id, cma_ledger_asset_type_t *asset_type,
        cma_ledger_retrieve_operation_t operation)
    # Retrieve/create an account
    int cma_ledger_retrieve_account(cma_ledger_t *ledger, cma_ledger_account_id_t *account_id,
        cma_ledger_account_t *account, void *addr_accid, cma_ledger_account_type_t *account_type,
        cma_ledger_retrieve_operation_t operation)
    # Deposit
    int cma_ledger_deposit(cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id,
        cma_ledger_account_id_t to_account_id, cma_amount_t *deposit)
    # Withdrawal
    int cma_ledger_withdraw(cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id,
        cma_ledger_account_id_t from_account_id, cma_amount_t *withdrawal)
    # Transfer
    int cma_ledger_transfer(cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id,
        cma_ledger_account_id_t from_account_id, cma_ledger_account_id_t to_account_id, cma_amount_t *amount)
    # Get balance
    int cma_ledger_get_balance(cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id,
        cma_ledger_account_id_t account_id, cma_amount_t *out_balance)
    # Get total supply
    int cma_ledger_get_total_supply(cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id,
        cma_amount_t *out_total_supply)
    # get error message
    char *cma_ledger_get_last_error_message()
