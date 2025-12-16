# Cartesi Machine Assets

The Cartesi Machine Assets is a C++ library to facilitate the creation and management of assets for the Cartesi Machine Rollups applications.

The API is divided into two main components

- parser: provides functions for common asset operations such as decoding operations and encoding deposits, withdrawals, and transfers.
- ledger: provides functions for managing asset ledgers and performing operations such as deposits, withdrawals, and transfers.

## Installation

To install, download the latest release from the [GitHub releases page](https://github.com/Mugen-Builders/machine-asset-tools/releases). It contains libraries and headers files (runtime and development environments) for musl and glibc systems.

If you are creating a Cartesi Rollups Application using Docker, you can add following command to your Dockerfile:

```dockerfile
ARG MACHINE_ASSET_TOOLS_VERSION
ADD https://github.com/Mugen-Builders/machine-asset-tools/releases/download/v${MACHINE_ASSET_TOOLS_VERSION}/machine-asset-tools_glibc_riscv64_v${MACHINE_ASSET_TOOLS_VERSION}.tar.gz /tmp/
RUN tar -xzf /tmp/machine-asset-tools_glibc_riscv64_v${MACHINE_ASSET_TOOLS_VERSION}.tar.gz -C / \
    && rm /tmp/machine-asset-tools_glibc_riscv64_v${MACHINE_ASSET_TOOLS_VERSION}.tar.gz
```

This will add the shared library to your Docker image. You can use the `_dev` artifact to add the headers and the static library.

## Usage

the library is divided into two parts: parser and ledger. The former helps to parse the input data, while the latter helps to manage the ledger state.

### Parser

The parser helps to parse the input data, which is in the form of an abi encoded byte string for advances a JSON-rpc-like string for inspects. The parser contains functions to decode the input and store the parsed data in a struct, and encode a voucher based pn a voucher request struct.

Ex. decode a Erc20 deposit:

```c
// input is a cmt_rollup_advance_t struct decoded from cmt_rollup_read_advance_state(rollup, &input)
cma_parser_input_t parser_input;

const int err = cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_DEPOSIT, &input, &parser_input);
if (err < 0) {
  printf("unable to decode erc20 deposit: %d - %s\n", -err, cma_parser_get_last_error_message());
}

parser_input.erc20_deposit.sender; // sender of the deposit
parser_input.erc20_deposit.token; // token address of the deposit
parser_input.erc20_deposit.amount; // amount of the deposit
parser_input.erc20_deposit.exec_layer_data; // exec layer data included on the deposit
```

Note: if the input is not a deposit, the parser can decode the input automatically by the functions selector of the payload. Use the the `CMA_PARSER_INPUT_TYPE_AUTO` type.

Explore the [parser's](/include/libcma/parser.h) to learn the function definitions and structs, the [sample applications](/sample_apps), and the [tests](/tests/parser.c) to see it in use.

### Ledger

The ledger manages the assets of accounts. It provides functions to deposit, transfer, and withdraw assets.

Ex. add a deposit to a new account:

```c
cma_ledger_t ledger;
if (!cma_ledger_init(&ledger)) {
  printf(cma_ledger_get_last_error_message());
}

// find/create account with ether address
cma_ledger_account_t account = {.address = {.data = {
  0xf3, 0x9f, 0xd6, 0xe5, 0x1a, 0xad, 0x88, 0xf6, 0xf4, 0xce, 0x6a, 0xb8, 0x82, 0x72, 0x79, 0xcf, 0xff, 0xb9, 0x22, 0x66,
}}};
cma_ledger_account_id_t account_id;
if (!cma_ledger_retrieve_account(&ledger, &account_id, &account, NULL, CMA_LEDGER_ACCOUNT_TYPE_ETHER_ADDRESS, CMA_LEDGER_OP_FIND_OR_CREATE)) {
  printf(cma_ledger_get_last_error_message());
}

// find/create asset with token address
cma_token_address_t token_address = {.address = {.data = {
  0x49, 0x16, 0x04, 0xc0, 0xfd, 0xf0, 0x83, 0x47, 0xdd, 0x1f, 0xa4, 0xee, 0x06, 0x2a, 0x82, 0x2a, 0x5d, 0xd0, 0x6b, 0x5d,
}}};
cma_ledger_asset_id_t asset_id;
if (!cma_ledger_retrieve_asset(&ledger, &asset_id, &token_address, NULL, CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS, CMA_LEDGER_OP_FIND_OR_CREATE)) {
  printf(cma_ledger_get_last_error_message());
}

// amount
cma_amount_t amount = {.data = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0xe0, 0xb6, 0xb3, 0xa7, 0x64, 0x00, 0x00,
}};

// deposit
if (!cma_ledger_deposit(&ledger, asset_id, account_id, &amount)) {
  printf(cma_ledger_get_last_error_message());
}

if (!cma_ledger_fini(&ledger)) {
  printf(cma_ledger_get_last_error_message());
}
```

The ledger uses an internal uint32 for the identification of the account and the assets. If enabled, every time you try to retrieve an asset or account that is not on the storage it creates a new one.

You can create an account without any account id, so it will never be mapped to any external identifier. You can also create an account with a an ether address, so it will be mapped to an external identifier, or a 32-byte string. Note that only accounts defined by ether addresses should be able to withdraw assets from the application.

The asset model defines a single model for all kinds of assets: it either has token address, token address and token id, or no token address and token id. Furthermore, the asset always may have any amount. In that sense, the differences between Ether, Erc20, Erc721, and Erc1155 are implemented on how you define the asset.

Explore the [ledger's](/include/libcma/ledger.h) to learn the function definitions and structs, the [sample application](/sample_apps/wallet_app/app.cpp), and the [tests](/tests/ledger.c) to see it in use.

## Reference

### Parser Function selectors

Advance:

```c
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
```

Note on transfer function: the receiver parameter from the transfer functions is a 32 bytes account id. This allows transfers for the "internal" accounts. Ether addresses should be converted to 32-byte strings when used in transfers.

Inspect methods:

```c
GET_BALANCE = "ledger_getBalance"
GET_TOTAL_SUPPLY = "ledger_getTotalSupply"
```

Full inspect example:

```javascript
{
  "method":"ledger_getBalance",
  "params":[
    "0xcb76129e0ed325e1e5c17f3f363bc2e93a227ecf", // required: 20-byte wallet address or 32-byte account id
    "0xc700000000000000000000000000000000000051", // optional: 20-byte token address
    "0x0711", // optional: 32-byte or relevant bytes token id
    "abc" // optional: exec layer data
  ]
}

{
  "method":"ledger_getTotalSupply",
  "params":[
    "0xc700000000000000000000000000000000000051", // optional: 20-byte token address
    "0x0711", // optional: 32-byte or relevant bytes token id
    "abc" // optional: exec layer data
  ]
}
```

### Parser Functions

```c
// decode advance
int cma_parser_decode_advance(cma_parser_input_type_t type, const cmt_rollup_advance_t *input,
    cma_parser_input_t *parser_input);

// decode inspect
int cma_parser_decode_inspect(cma_parser_input_type_t type, const cmt_rollup_inspect_t *input,
    cma_parser_input_t *parser_input);

// encode voucher
int cma_parser_encode_voucher(cma_parser_voucher_type_t type, const cma_abi_address_t *app_address,
    const cma_parser_voucher_data_t *voucher_request, cma_voucher_t *voucher);

// get error message
const char *cma_parser_get_last_error_message();
```

### Ledger Functions

```c
// manage ledger class
int cma_ledger_init(cma_ledger_t *ledger);
int cma_ledger_fini(cma_ledger_t *ledger);
int cma_ledger_reset(cma_ledger_t *ledger);

// Retrieve/create an asset
int cma_ledger_retrieve_asset(cma_ledger_t *ledger, cma_ledger_asset_id_t *asset_id,
    cma_token_address_t *token_address, cma_token_id_t *token_id, cma_ledger_asset_type_t asset_type,
    cma_ledger_retrieve_operation_t operation);

// Retrieve/create an account
int cma_ledger_retrieve_account(cma_ledger_t *ledger, cma_ledger_account_id_t *account_id,
    cma_ledger_account_t *account, const void *addr_accid, cma_ledger_account_type_t account_type,
    cma_ledger_retrieve_operation_t operation);

// Deposit
int cma_ledger_deposit(cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id,
    cma_ledger_account_id_t to_account_id, const cma_amount_t *deposit);

// Withdrawal
int cma_ledger_withdraw(cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id,
    cma_ledger_account_id_t from_account_id, const cma_amount_t *withdrawal);

// Transfer
int cma_ledger_transfer(cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id,
    cma_ledger_account_id_t from_account_id, cma_ledger_account_id_t to_account_id, const cma_amount_t *amount);

// Get balance
int cma_ledger_get_balance(cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id,
    cma_ledger_account_id_t account_id, cma_amount_t *out_balance);

// Get total supply
int cma_ledger_get_total_supply(cma_ledger_t *ledger, cma_ledger_asset_id_t asset_id,
    cma_amount_t *out_total_supply);

// get error message
const char *cma_ledger_get_last_error_message();
```
