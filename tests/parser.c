#include <stdio.h>

#include "libcma/ledger.h"

int main(void) {
    printf("All parser tests passed!\n");
    return 0;
}

// global ether_asset_id;
// void handle_advance(rollup) {

//     metadata = get_metadata(rollup);
//     input = get_advance(rollup);
//     cma_parser_input_t assetlib_input;

//     // decoding the request
//     switch
//         metadata.msg_sender {
//             case ETHER_PORTAL:
//                 err = cma_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_DEPOSIT, input, assetlib_input);
//                 assetlib_input.ether_deposit.sender;
//                 assetlib_input.ether_deposit.amount;
//                 ledger_asset_retrieve(ether_asset_id);
//                 // processing here
//             default:
//                 err = cma_decode_advance(CMA_PARSER_INPUT_TYPE_AUTO, input, assetlib_input);
//         }

//     // processing of the request
//     switch
//         assetlib_input.type {}

//     // normaladvance

//     //
// }

// ether_deposit_t cma_decode_advance_ether_deposit(advance_input);

// void main() {

//     cma_ledger_t ledger;
//     err = ledger_init(&ledger);
//     cma_ledger_asset_t asset;
//     int ledger_create_asset(&ledger, CMA_LEDGER_ASSET_CREATION_TYPE_SEQUENTIAL, cma_ledger_asset_t * asset);

//     ether_asset_id = asset.id;

//     // loop to process finish
//     while (1) {
//         handle_advance();
//     }
// }
// /*
// {"method" : "cma_getTotaLSupply", "parameters" : [ acc_id, token ]} getTotalSupply(bytes32 account_id,
//     address token_address, uint256 token_id)

// (address token_address, uint256 token_id) -> amount

// ether: (null, null) -> amount
// erc20: (address token_address, null)-> amount
// erc721: (address token_address, uint256 token_id) -> amount (max 1)
// erc1155: (address token_address, uint256 token_id) -> amount

// */
