#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libcma/ledger.h"

void test_init_and_fini(void) {
    assert(cma_ledger_fini(NULL) == -EINVAL);

    cma_ledger_t ledger;

    assert(cma_ledger_fini(&ledger) == -EINVAL);

    assert(cma_ledger_init(&ledger) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);

    printf("%s passed\n", __FUNCTION__);
}

void test_init_and_reset(void) {
    assert(cma_ledger_reset(NULL) == -EINVAL);

    cma_ledger_t ledger;
    cma_ledger_reset(&ledger);
    assert(cma_ledger_reset(&ledger) == -EINVAL);

    assert(cma_ledger_init(&ledger) == CMA_LEDGER_SUCCESS);

    // add assets and accounts and test not empty

    cma_ledger_asset_id_t asset_id;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, CMA_LEDGER_ASSET_TYPE_ID,
               CMA_LEDGER_OP_FIND_OR_CREATE) == CMA_LEDGER_SUCCESS);

    assert(asset_id == 0);

    // reset and test empty
    assert(cma_ledger_reset(&ledger) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, CMA_LEDGER_ASSET_TYPE_ID, CMA_LEDGER_OP_FIND) ==
        CMA_LEDGER_ERROR_ASSET_NOT_FOUND);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);
    printf("%s passed\n", __FUNCTION__);
}

void test_asset_id(void) {
    cma_ledger_t ledger;
    assert(cma_ledger_init(&ledger) == CMA_LEDGER_SUCCESS);

    assert(
        cma_ledger_retrieve_asset(&ledger, NULL, NULL, NULL, CMA_LEDGER_ASSET_TYPE_ID, CMA_LEDGER_OP_FIND) == -EINVAL);

    cma_ledger_asset_id_t asset_id = 0;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, CMA_LEDGER_ASSET_TYPE_ID, CMA_LEDGER_OP_FIND) ==
        CMA_LEDGER_ERROR_ASSET_NOT_FOUND);

    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, CMA_LEDGER_ASSET_TYPE_ID, CMA_LEDGER_OP_CREATE) ==
        CMA_LEDGER_SUCCESS);

    assert(asset_id == 0);

    asset_id += 2;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, CMA_LEDGER_ASSET_TYPE_ID,
               CMA_LEDGER_OP_FIND_OR_CREATE) == CMA_LEDGER_SUCCESS);

    assert(asset_id == 1);

    asset_id = 0;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, CMA_LEDGER_ASSET_TYPE_ID, CMA_LEDGER_OP_FIND) ==
        CMA_LEDGER_SUCCESS);

    asset_id = 0;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, CMA_LEDGER_ASSET_TYPE_ID,
               CMA_LEDGER_OP_FIND_OR_CREATE) == CMA_LEDGER_SUCCESS);
    assert(asset_id == 0);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);
    printf("%s passed\n", __FUNCTION__);
}

void test_asset_with_token_address(void) {
    cma_ledger_t ledger;
    assert(cma_ledger_init(&ledger) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_retrieve_asset(&ledger, NULL, NULL, NULL, CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS,
               CMA_LEDGER_OP_FIND) == -EINVAL);

    // clang-format off
    cma_token_address_t token_address1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    }};
    // clang-format on

    assert(cma_ledger_retrieve_asset(&ledger, NULL, &token_address1, NULL, CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS,
               CMA_LEDGER_OP_FIND) == CMA_LEDGER_ERROR_ASSET_NOT_FOUND);

    cma_ledger_asset_id_t asset_id;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, &token_address1, NULL, CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);

    assert(asset_id == 0);

    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, &token_address1, NULL, CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_ERROR_INSERTION_ERROR);

    // clang-format off
    cma_token_address_t token_address2 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    }};
    // clang-format on

    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, &token_address2, NULL, CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS,
               CMA_LEDGER_OP_FIND_OR_CREATE) == CMA_LEDGER_SUCCESS);

    assert(asset_id == 1);

    cma_ledger_asset_id_t asset_id_find;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id_find, &token_address1, NULL,
               CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS, CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);

    assert(asset_id_find == 0);

    cma_ledger_asset_id_t asset_id_to_find = 0;
    cma_token_address_t token_address_find;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id_to_find, &token_address_find, NULL, CMA_LEDGER_ASSET_TYPE_ID,
               CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);

    assert(memcmp(token_address_find.data, token_address1.data, CMA_ABI_ADDRESS_LENGTH) == 0);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);
    printf("%s passed\n", __FUNCTION__);
}

void test_asset_with_token_address_and_id(void) {
    cma_ledger_t ledger;
    assert(cma_ledger_init(&ledger) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_retrieve_asset(&ledger, NULL, NULL, NULL, CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID,
               CMA_LEDGER_OP_FIND) == -EINVAL);

    // clang-format off
    cma_token_address_t token_address1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    }};
    // clang-format on

    assert(cma_ledger_retrieve_asset(&ledger, NULL, &token_address1, NULL, CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID,
               CMA_LEDGER_OP_FIND) == -EINVAL);

    // clang-format off
    cma_token_id_t token_id1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01,
    }};
    // clang-format on

    assert(cma_ledger_retrieve_asset(&ledger, NULL, &token_address1, &token_id1, CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID,
               CMA_LEDGER_OP_FIND) == CMA_LEDGER_ERROR_ASSET_NOT_FOUND);

    cma_ledger_asset_id_t asset_id;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, &token_address1, &token_id1,
               CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID, CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);

    assert(asset_id == 0);

    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, &token_address1, &token_id1,
               CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID, CMA_LEDGER_OP_CREATE) == CMA_LEDGER_ERROR_INSERTION_ERROR);

    // clang-format off
    cma_token_id_t token_id2 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x02,
    }};
    // clang-format on

    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, &token_address1, &token_id2,
               CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID, CMA_LEDGER_OP_FIND_OR_CREATE) == CMA_LEDGER_SUCCESS);

    assert(asset_id == 1);

    cma_ledger_asset_id_t asset_id_find;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id_find, &token_address1, &token_id1,
               CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID, CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);

    assert(asset_id_find == 0);

    cma_ledger_asset_id_t asset_id_to_find = 0;
    cma_token_address_t token_address_find;
    cma_token_id_t token_id_find;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id_to_find, &token_address_find, &token_id_find,
               CMA_LEDGER_ASSET_TYPE_ID, CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);

    assert(memcmp(token_address_find.data, token_address1.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp(token_id_find.data, token_id1.data, CMA_ABI_ADDRESS_LENGTH) == 0);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);
    printf("%s passed\n", __FUNCTION__);
}

void test_account_id(void) {
    cma_ledger_t ledger;
    assert(cma_ledger_init(&ledger) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_retrieve_account(&ledger, NULL, NULL, NULL, CMA_LEDGER_ACCOUNT_TYPE_ID, CMA_LEDGER_OP_FIND) ==
        -EINVAL);

    cma_ledger_account_id_t account_id = 0;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, CMA_LEDGER_ACCOUNT_TYPE_ID,
               CMA_LEDGER_OP_FIND) == CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);

    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, CMA_LEDGER_ACCOUNT_TYPE_ID,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);

    assert(account_id == 0);

    account_id += 2;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, CMA_LEDGER_ACCOUNT_TYPE_ID,
               CMA_LEDGER_OP_FIND_OR_CREATE) == CMA_LEDGER_SUCCESS);

    assert(account_id == 1);

    account_id = 0;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, CMA_LEDGER_ACCOUNT_TYPE_ID,
               CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);

    account_id = 0;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, CMA_LEDGER_ACCOUNT_TYPE_ID,
               CMA_LEDGER_OP_FIND_OR_CREATE) == CMA_LEDGER_SUCCESS);
    assert(account_id == 0);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);
    printf("%s passed\n", __FUNCTION__);
}

void test_account_address(void) {
    cma_ledger_t ledger;
    assert(cma_ledger_init(&ledger) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_retrieve_account(&ledger, NULL, NULL, NULL, CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS,
               CMA_LEDGER_OP_FIND) == -EINVAL);

    // clang-format off
    cma_ledger_account_t account1 = {.address = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    }}};
    // clang-format on;
    assert(cma_ledger_retrieve_account(&ledger, NULL, &account1, NULL, CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS,
                CMA_LEDGER_OP_FIND) == CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);

    cma_ledger_account_id_t account_id;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, &account1, NULL, CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);
    assert(account_id == 0);

    assert(cma_ledger_retrieve_account(&ledger, &account_id, &account1, NULL, CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_ERROR_INSERTION_ERROR);

    // clang-format off
    cma_ledger_account_t account2 = {.address = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    }}};
    // clang-format on;

    assert(cma_ledger_retrieve_account(&ledger, &account_id, &account2, NULL, CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);
    assert(account_id == 1);

    cma_ledger_account_id_t account_id_found;
    assert(cma_ledger_retrieve_account(&ledger, &account_id_found, &account1, NULL,
CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS, CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS); assert(account_id_found == 0);

    cma_ledger_account_id_t account_id_to_find = 0;
    cma_ledger_account_t account_found;
    assert(cma_ledger_retrieve_account(&ledger, &account_id_to_find, &account_found, NULL, CMA_LEDGER_ACCOUNT_TYPE_ID,
               CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);
    assert(memcmp(account_found.address.data, account1.address.data, CMA_ABI_ADDRESS_LENGTH) == 0);

    // clang-format off
    cma_ledger_account_t account_to_find = {.account_id = {.data = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
        0x0b, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01,
    }}};
    // clang-format on;

    account_id_found = 99;
    assert(cma_ledger_retrieve_account(&ledger, &account_id_found, &account_to_find, NULL,
CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS, CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS); assert(account_id_found == 0);

    cma_ledger_account_t account_found2 = {};
    assert(cma_ledger_retrieve_account(&ledger, &account_id_to_find, &account_found2, NULL,
CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS, CMA_LEDGER_OP_FIND) == CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);

    // clang-format off
    cma_abi_address_t address2 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    }};
    // clang-format on;

    assert(cma_ledger_retrieve_account(&ledger, NULL, NULL, &address2, CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS,
               CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_retrieve_account(&ledger, &account_id_found, NULL, &address2,
CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS, CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS); assert(account_id_found == 1);

    assert(cma_ledger_retrieve_account(&ledger, NULL, &account_found2, &address2, CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS,
               CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);
    assert(memcmp(account_found2.address.data, account2.address.data, CMA_ABI_ADDRESS_LENGTH) == 0);

    assert(cma_ledger_retrieve_account(&ledger, &account_id_found, &account_found2, &address2,
CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS, CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS); assert(account_id_found == 1);
    assert(memcmp(account_found2.address.data, account2.address.data, CMA_ABI_ADDRESS_LENGTH) == 0);

    // clang-format off
    cma_abi_address_t address3 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
    }};
    // clang-format on;

    cma_ledger_account_id_t account_id_created;
    cma_ledger_account_t account_created;
    assert(cma_ledger_retrieve_account(&ledger, &account_id_created, &account_created, &address3,
CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS, CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS); assert(account_id_created == 2);
    assert(memcmp(account_created.address.data, address3.data, CMA_ABI_ADDRESS_LENGTH) == 0);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);
    printf("%s passed\n", __FUNCTION__);
}

void test_account_full_id(void) {
    cma_ledger_t ledger;
    assert(cma_ledger_init(&ledger) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_retrieve_account(&ledger, NULL, NULL, NULL, CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID,
               CMA_LEDGER_OP_FIND) == -EINVAL);

    // clang-format off
    cma_ledger_account_t account1 = {.account_id = {.data = {
        0xde, 0xad, 0xbe, 0xef, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01,
    }}};
    // clang-format on;
    assert(cma_ledger_retrieve_account(&ledger, NULL, &account1, NULL, CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID,
                CMA_LEDGER_OP_FIND) == CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);

    cma_ledger_account_id_t account_id;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, &account1, NULL, CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);
    assert(account_id == 0);

    assert(cma_ledger_retrieve_account(&ledger, &account_id, &account1, NULL, CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_ERROR_INSERTION_ERROR);

    // clang-format off
    cma_ledger_account_t account2 = {.account_id = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x02,
    }}};
    // clang-format on;

    assert(cma_ledger_retrieve_account(&ledger, &account_id, &account2, NULL, CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);
    assert(account_id == 1);

    cma_ledger_account_id_t account_id_found;
    assert(cma_ledger_retrieve_account(&ledger, &account_id_found, &account1, NULL, CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID,
               CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);
    assert(account_id_found == 0);

    cma_ledger_account_id_t account_id_to_find = 0;
    cma_ledger_account_t account_found;
    assert(cma_ledger_retrieve_account(&ledger, &account_id_to_find, &account_found, NULL, CMA_LEDGER_ACCOUNT_TYPE_ID,
               CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);
    assert(memcmp(account_found.address.data, account1.address.data, CMA_ABI_ADDRESS_LENGTH) == 0);

    // clang-format off
    cma_ledger_account_t account_to_find = {.address = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    }}};
    // clang-format on;

    assert(cma_ledger_retrieve_account(&ledger, &account_id_found, &account_to_find, NULL,
CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS, CMA_LEDGER_OP_FIND) == CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);

    // clang-format off
    cma_ledger_account_t account_to_find2 = {.address = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    }}};
    // clang-format on;

    // full id 2 is has initial 10 bytes zeroed
    assert(cma_ledger_retrieve_account(&ledger, &account_id_found, &account_to_find2, NULL,
CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS, CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS); assert(account_id_found == 1);

    cma_ledger_account_t account_found2 = {};
    assert(cma_ledger_retrieve_account(&ledger, &account_id_to_find, &account_found2, NULL,
CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID, CMA_LEDGER_OP_FIND) == CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);

    // clang-format off
    cma_account_id_t full_account2 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x02,
    }};
    // clang-format on;

    assert(cma_ledger_retrieve_account(&ledger, NULL, NULL, &full_account2, CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID,
               CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_retrieve_account(&ledger, &account_id_found, NULL, &full_account2,
CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID, CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS); assert(account_id_found == 1);

    assert(cma_ledger_retrieve_account(&ledger, NULL, &account_found2, &full_account2,
CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID, CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);
    assert(memcmp(account_found2.account_id.data, account2.account_id.data, CMA_ABI_ADDRESS_LENGTH) == 0);

    assert(cma_ledger_retrieve_account(&ledger, &account_id_found, &account_found2, &full_account2,
CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID, CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS); assert(account_id_found == 1);
    assert(memcmp(account_found2.account_id.data, account2.account_id.data, CMA_ABI_ADDRESS_LENGTH) == 0);

    // clang-format off
    cma_account_id_t full_account3 = {.data = {
        0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x03,
    }};
    // clang-format on;

    cma_ledger_account_id_t account_id_created;
    cma_ledger_account_t account_created;
    assert(cma_ledger_retrieve_account(&ledger, &account_id_created, &account_created, &full_account3,
CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID, CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS); assert(account_id_created == 2);
    assert(memcmp(account_created.account_id.data, full_account3.data, CMA_ABI_ADDRESS_LENGTH) == 0);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);
    printf("%s passed\n", __FUNCTION__);
}

void test_asset_total_supply_balance(void) {
    cma_ledger_t ledger;
    assert(cma_ledger_init(&ledger) == CMA_LEDGER_SUCCESS);

    cma_ledger_asset_id_t asset_id;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, CMA_LEDGER_ASSET_TYPE_ID, CMA_LEDGER_OP_CREATE) ==
        CMA_LEDGER_SUCCESS);

    cma_amount_t out_total_supply = {};
    assert(cma_ledger_get_total_supply(&ledger, 1000, &out_total_supply) ==
        CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
    assert(cma_ledger_get_total_supply(&ledger, asset_id, &out_total_supply) ==
        CMA_LEDGER_SUCCESS);

    // clang-format off
    cma_amount_t zero_amount = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00,
    }};
    // clang-format on;

    assert(memcmp(out_total_supply.data, zero_amount.data, CMA_ABI_U256_LENGTH) == 0);

    cma_amount_t balance = {};
    assert(cma_ledger_get_balance(&ledger, 1000, 1000, &balance) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(balance.data, zero_amount.data, CMA_ABI_U256_LENGTH) == 0);

    assert(cma_ledger_get_balance(&ledger, asset_id, 1000, &balance) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(balance.data, zero_amount.data, CMA_ABI_U256_LENGTH) == 0);

    cma_ledger_account_id_t account_id;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, CMA_LEDGER_ACCOUNT_TYPE_ID,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_get_balance(&ledger, asset_id, account_id, &balance) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(balance.data, zero_amount.data, CMA_ABI_U256_LENGTH) == 0);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);
    printf("%s passed\n", __FUNCTION__);
}

void test_deposit(void) {
    cma_ledger_t ledger;
    assert(cma_ledger_init(&ledger) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_deposit(&ledger, 1000, 1000, NULL) ==
        -EINVAL);

    // clang-format off
    cma_amount_t amount = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
        0x00, 0x04,
    }};
    // clang-format on;

    assert(cma_ledger_deposit(&ledger, 1000, 1000, &amount) ==
        CMA_LEDGER_ERROR_ASSET_NOT_FOUND);

    cma_ledger_asset_id_t asset_id;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, CMA_LEDGER_ASSET_TYPE_ID, CMA_LEDGER_OP_CREATE) ==
        CMA_LEDGER_SUCCESS);

    assert(cma_ledger_deposit(&ledger, asset_id, 1000, &amount) ==
        CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);

    cma_ledger_account_id_t account_id;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, CMA_LEDGER_ACCOUNT_TYPE_ID,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_deposit(&ledger, 1000, account_id, &amount) ==
        CMA_LEDGER_ERROR_ASSET_NOT_FOUND);

    assert(cma_ledger_deposit(&ledger, asset_id, account_id, &amount) ==
        CMA_LEDGER_SUCCESS);

    cma_amount_t supply = {};
    assert(cma_ledger_get_total_supply(&ledger, asset_id, &supply) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(supply.data, amount.data, CMA_ABI_U256_LENGTH) == 0);

    cma_amount_t balance = {};
    assert(cma_ledger_get_balance(&ledger, asset_id, account_id, &balance) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(balance.data, amount.data, CMA_ABI_U256_LENGTH) == 0);

    // clang-format off
    cma_amount_t amount2 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20,
        0x00, 0x08,
    }};
    // clang-format on;

    assert(cma_ledger_deposit(&ledger, asset_id, account_id, &amount) ==
        CMA_LEDGER_SUCCESS);

    assert(cma_ledger_get_total_supply(&ledger, asset_id, &supply) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(supply.data, amount2.data, CMA_ABI_U256_LENGTH) == 0);

    assert(cma_ledger_get_balance(&ledger, asset_id, account_id, &balance) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(balance.data, amount2.data, CMA_ABI_U256_LENGTH) == 0);

    // clang-format off
    cma_amount_t amount4 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
        0x00, 0x10,
    }};
    // clang-format on;

    cma_ledger_account_id_t account_id2;
    assert(cma_ledger_retrieve_account(&ledger, &account_id2, NULL, NULL, CMA_LEDGER_ACCOUNT_TYPE_ID,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_deposit(&ledger, asset_id, account_id2, &amount) ==
        CMA_LEDGER_SUCCESS);
    assert(cma_ledger_deposit(&ledger, asset_id, account_id2, &amount) ==
        CMA_LEDGER_SUCCESS);

    assert(cma_ledger_get_total_supply(&ledger, asset_id, &supply) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(supply.data, amount4.data, CMA_ABI_U256_LENGTH) == 0);

    assert(cma_ledger_get_balance(&ledger, asset_id, account_id, &balance) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(balance.data, amount2.data, CMA_ABI_U256_LENGTH) == 0);

    assert(cma_ledger_get_balance(&ledger, asset_id, account_id2, &balance) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(balance.data, amount2.data, CMA_ABI_U256_LENGTH) == 0);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);
    printf("%s passed\n", __FUNCTION__);
}

void test_withdraw(void) {
    cma_ledger_t ledger;
    assert(cma_ledger_init(&ledger) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_withdraw(&ledger, 1000, 1000, NULL) ==
        -EINVAL);

    // clang-format off
    cma_amount_t amount = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
        0x00, 0x04,
    }};
    // clang-format on;

    assert(cma_ledger_withdraw(&ledger, 1000, 1000, &amount) ==
        CMA_LEDGER_ERROR_ASSET_NOT_FOUND);

    cma_ledger_asset_id_t asset_id;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, CMA_LEDGER_ASSET_TYPE_ID, CMA_LEDGER_OP_CREATE) ==
        CMA_LEDGER_SUCCESS);

    assert(cma_ledger_withdraw(&ledger, asset_id, 1000, &amount) ==
        CMA_LEDGER_ERROR_SUPPLY_OVERFLOW);

    cma_ledger_account_id_t account_id;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, CMA_LEDGER_ACCOUNT_TYPE_ID,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_withdraw(&ledger, 1000, account_id, &amount) ==
        CMA_LEDGER_ERROR_ASSET_NOT_FOUND);

    assert(cma_ledger_deposit(&ledger, asset_id, account_id, &amount) ==
        CMA_LEDGER_SUCCESS);

    cma_ledger_account_id_t account_id2;
    assert(cma_ledger_retrieve_account(&ledger, &account_id2, NULL, NULL, CMA_LEDGER_ACCOUNT_TYPE_ID,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_withdraw(&ledger, asset_id, account_id2, &amount) ==
        CMA_LEDGER_ERROR_INSUFFICIENT_FUNDS);

    // clang-format off
    cma_amount_t amount2 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20,
        0x00, 0x08,
    }};
    // clang-format on;

    assert(cma_ledger_deposit(&ledger, asset_id, account_id, &amount2) ==
        CMA_LEDGER_SUCCESS);
    assert(cma_ledger_withdraw(&ledger, asset_id, account_id, &amount) ==
        CMA_LEDGER_SUCCESS);

    cma_amount_t supply = {};
    assert(cma_ledger_get_total_supply(&ledger, asset_id, &supply) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(supply.data, amount2.data, CMA_ABI_U256_LENGTH) == 0);

    cma_amount_t balance = {};
    assert(cma_ledger_get_balance(&ledger, asset_id, account_id, &balance) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(balance.data, amount2.data, CMA_ABI_U256_LENGTH) == 0);

    // clang-format off
    cma_amount_t amount4 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
        0x00, 0x10,
    }};
    // clang-format on;

    assert(cma_ledger_deposit(&ledger, asset_id, account_id2, &amount4) ==
        CMA_LEDGER_SUCCESS);

    assert(cma_ledger_withdraw(&ledger, asset_id, account_id2, &amount2) ==
        CMA_LEDGER_SUCCESS);

    assert(cma_ledger_get_total_supply(&ledger, asset_id, &supply) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(supply.data, amount4.data, CMA_ABI_U256_LENGTH) == 0);

    assert(cma_ledger_get_balance(&ledger, asset_id, account_id, &balance) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(balance.data, amount2.data, CMA_ABI_U256_LENGTH) == 0);

    assert(cma_ledger_get_balance(&ledger, asset_id, account_id2, &balance) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(balance.data, amount2.data, CMA_ABI_U256_LENGTH) == 0);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);
    printf("%s passed\n", __FUNCTION__);
}

void test_transfer(void) {
    cma_ledger_t ledger;
    assert(cma_ledger_init(&ledger) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_transfer(&ledger, 1000, 1000, 1001, NULL) ==
        -EINVAL);

    // clang-format off
    cma_amount_t amount = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
        0x00, 0x04,
    }};
    // clang-format on;

    assert(cma_ledger_transfer(&ledger, 1000, 1000, 1001, &amount) ==
        CMA_LEDGER_ERROR_ASSET_NOT_FOUND);

    cma_ledger_asset_id_t asset_id;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, CMA_LEDGER_ASSET_TYPE_ID, CMA_LEDGER_OP_CREATE) ==
        CMA_LEDGER_SUCCESS);

    assert(cma_ledger_transfer(&ledger, asset_id, 1000, 1000, &amount) ==
        -EINVAL);

    cma_ledger_account_id_t account_id;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, CMA_LEDGER_ACCOUNT_TYPE_ID,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_transfer(&ledger, 1000, account_id, 1000, &amount) ==
        CMA_LEDGER_ERROR_ASSET_NOT_FOUND);

    assert(cma_ledger_deposit(&ledger, asset_id, account_id, &amount) ==
        CMA_LEDGER_SUCCESS);

    assert(cma_ledger_transfer(&ledger, asset_id, account_id, account_id, &amount) ==
        -EINVAL);

    assert(cma_ledger_transfer(&ledger, asset_id, account_id, 1000, &amount) ==
        CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);

    cma_ledger_account_id_t account_id2;
    assert(cma_ledger_retrieve_account(&ledger, &account_id2, NULL, NULL, CMA_LEDGER_ACCOUNT_TYPE_ID,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_transfer(&ledger, asset_id, account_id2, account_id, &amount) ==
        CMA_LEDGER_ERROR_INSUFFICIENT_FUNDS);

    // clang-format off
    cma_amount_t amount2 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20,
        0x00, 0x08,
    }};
    // clang-format on;

    assert(cma_ledger_deposit(&ledger, asset_id, account_id, &amount) ==
        CMA_LEDGER_SUCCESS);
    assert(cma_ledger_deposit(&ledger, asset_id, account_id2, &amount2) ==
        CMA_LEDGER_SUCCESS);

    assert(cma_ledger_transfer(&ledger, asset_id, account_id, account_id2, &amount) ==
        CMA_LEDGER_SUCCESS);

    // clang-format off
    cma_amount_t amount4 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
        0x00, 0x10,
    }};
    // clang-format on;

    cma_amount_t supply = {};
    assert(cma_ledger_get_total_supply(&ledger, asset_id, &supply) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(supply.data, amount4.data, CMA_ABI_U256_LENGTH) == 0);

    // clang-format off
    cma_amount_t amount3 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30,
        0x00, 0x0c,
    }};
    // clang-format on;

    cma_amount_t balance = {};
    assert(cma_ledger_get_balance(&ledger, asset_id, account_id, &balance) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(balance.data, amount.data, CMA_ABI_U256_LENGTH) == 0);
    assert(cma_ledger_get_balance(&ledger, asset_id, account_id2, &balance) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(balance.data, amount3.data, CMA_ABI_U256_LENGTH) == 0);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);
    printf("%s passed\n", __FUNCTION__);
}

int main(void) {
    test_init_and_fini();
    test_init_and_reset();
    test_asset_id();
    test_asset_with_token_address();
    test_asset_with_token_address_and_id();
    test_account_id();
    test_account_address();
    test_account_full_id();
    test_asset_total_supply_balance();
    test_deposit();
    test_withdraw();
    test_transfer();
    printf("All ledger tests passed!\n");
    return 0;
}
