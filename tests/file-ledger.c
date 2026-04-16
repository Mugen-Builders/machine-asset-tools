#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libcma/ledger.h"

#define MAX_ACCOUNTS 16UL * 1024      //< Maximum number of accounts.
#define MAX_BALANCES 8 * MAX_ACCOUNTS //< Max balances
#define MAX_ASSETS 8UL                //< Maximum number of assets.
#define MEM_LENGTH 64UL * 1024 * 1024 //< State length
#define FILE_SIZE 1 * MEM_LENGTH      //< State file size
#define TMPFILE_PATH_SIZE 15

int create_temp_file(char *filepath_template, size_t size) {
    int fd = mkstemp(filepath_template);
    if (fd == -1) {
        return -EBADF;
    }
    if (ftruncate(fd, size) == -1) {
        close(fd);
        unlink(filepath_template);
        return -EBADFD;
    }
    close(fd);
    return 0;
}

void test_init_and_fini(void) {
    char temp_filepath[TMPFILE_PATH_SIZE] = "/tmp/tmpXXXXXX";
    assert(create_temp_file(temp_filepath, FILE_SIZE) == 0);

    cma_ledger_t ledger;

    assert(cma_ledger_fini(&ledger) == -EINVAL);

    assert(cma_ledger_init_file(&ledger, temp_filepath, CMA_LEDGER_CREATE_ONLY, 0, MEM_LENGTH, 2 * MAX_ACCOUNTS,
               2 * MAX_ASSETS, 2 * MAX_BALANCES) == -ENOBUFS);

    assert(cma_ledger_init_file(&ledger, temp_filepath, CMA_LEDGER_CREATE_ONLY, 0, MEM_LENGTH, MAX_ACCOUNTS, MAX_ASSETS,
               MAX_BALANCES) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_init_file(&ledger, temp_filepath, CMA_LEDGER_OPEN_ONLY, 0, MEM_LENGTH, MAX_ACCOUNTS, MAX_ASSETS,
               MAX_BALANCES) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);

    assert(unlink(temp_filepath) == 0);
    printf("%s passed\n", __FUNCTION__);
}

void test_init_and_reset(void) {
    char temp_filepath[TMPFILE_PATH_SIZE] = "/tmp/tmpXXXXXX";
    assert(create_temp_file(temp_filepath, FILE_SIZE) == 0);
    assert(cma_ledger_reset(NULL) == -EINVAL);

    cma_ledger_t ledger;

    assert(cma_ledger_init_file(&ledger, temp_filepath, CMA_LEDGER_CREATE_ONLY, 0, FILE_SIZE, MAX_ACCOUNTS, MAX_ASSETS,
               MAX_BALANCES) == CMA_LEDGER_SUCCESS);

    // add assets and accounts and test not empty

    cma_ledger_asset_id_t asset_id;
    cma_ledger_asset_type_t asset_type = CMA_LEDGER_ASSET_TYPE_ID;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND_OR_CREATE) ==
        CMA_LEDGER_SUCCESS);

    assert(asset_id == 0);

    // reset and test empty
    assert(cma_ledger_reset(&ledger) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND) ==
        CMA_LEDGER_ERROR_ASSET_NOT_FOUND);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);
    assert(unlink(temp_filepath) == 0);
    printf("%s passed\n", __FUNCTION__);
}

void test_init_and_load(void) {
    char temp_filepath[TMPFILE_PATH_SIZE] = "/tmp/tmpXXXXXX";
    assert(create_temp_file(temp_filepath, FILE_SIZE) == 0);
    assert(cma_ledger_reset(NULL) == -EINVAL);

    cma_ledger_t ledger;
    cma_ledger_reset(&ledger);
    assert(cma_ledger_reset(&ledger) == -EINVAL);

    assert(cma_ledger_init_file(&ledger, temp_filepath, CMA_LEDGER_CREATE_ONLY, 0, FILE_SIZE, MAX_ACCOUNTS, MAX_ASSETS,
               MAX_BALANCES) == CMA_LEDGER_SUCCESS);

    // add assets and accounts and test not empty
    cma_ledger_asset_id_t asset_id;
    cma_ledger_asset_type_t asset_type = CMA_LEDGER_ASSET_TYPE_ID;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_CREATE) ==
        CMA_LEDGER_SUCCESS);
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_CREATE) ==
        CMA_LEDGER_SUCCESS);

    assert(asset_id == 1);

    // finalize and load
    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);

    cma_ledger_t ledger2;
    assert(cma_ledger_init_file(&ledger2, temp_filepath, CMA_LEDGER_OPEN_ONLY, 0, FILE_SIZE, MAX_ACCOUNTS, MAX_ASSETS,
               MAX_BALANCES) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_retrieve_asset(&ledger2, &asset_id, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND) ==
        CMA_LEDGER_SUCCESS);

    assert(cma_ledger_fini(&ledger2) == CMA_LEDGER_SUCCESS);
    assert(unlink(temp_filepath) == 0);
    printf("%s passed\n", __FUNCTION__);
}

void test_asset_id(void) {
    char temp_filepath[TMPFILE_PATH_SIZE] = "/tmp/tmpXXXXXX";
    assert(create_temp_file(temp_filepath, FILE_SIZE) == 0);
    cma_ledger_t ledger;
    assert(cma_ledger_init_file(&ledger, temp_filepath, CMA_LEDGER_CREATE_ONLY, 0, FILE_SIZE, MAX_ACCOUNTS, MAX_ASSETS,
               MAX_BALANCES) == CMA_LEDGER_SUCCESS);

    cma_ledger_asset_type_t asset_type = CMA_LEDGER_ASSET_TYPE_ID;
    assert(cma_ledger_retrieve_asset(&ledger, NULL, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND) == -EINVAL);

    cma_ledger_asset_id_t asset_id = 0;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND) ==
        CMA_LEDGER_ERROR_ASSET_NOT_FOUND);

    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_CREATE) ==
        CMA_LEDGER_SUCCESS);

    assert(asset_id == 0);

    asset_id += 2;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND_OR_CREATE) ==
        CMA_LEDGER_SUCCESS);

    assert(asset_id == 1);

    asset_id = 0;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND) ==
        CMA_LEDGER_SUCCESS);

    asset_id = 0;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND_OR_CREATE) ==
        CMA_LEDGER_SUCCESS);
    assert(asset_id == 0);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);
    assert(unlink(temp_filepath) == 0);
    printf("%s passed\n", __FUNCTION__);
}

void test_asset_base(void) {
    char temp_filepath[TMPFILE_PATH_SIZE] = "/tmp/tmpXXXXXX";
    assert(create_temp_file(temp_filepath, FILE_SIZE) == 0);
    cma_ledger_t ledger;
    assert(cma_ledger_init_file(&ledger, temp_filepath, CMA_LEDGER_CREATE_ONLY, 0, FILE_SIZE, MAX_ACCOUNTS, MAX_ASSETS,
               MAX_BALANCES) == CMA_LEDGER_SUCCESS);

    cma_ledger_asset_type_t asset_type = CMA_LEDGER_ASSET_TYPE_BASE;
    assert(cma_ledger_retrieve_asset(&ledger, NULL, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND) == -EINVAL);

    cma_ledger_asset_id_t asset_id = 0;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND) ==
        CMA_LEDGER_ERROR_ASSET_NOT_FOUND);

    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_CREATE) ==
        CMA_LEDGER_SUCCESS);

    assert(asset_id == 0);

    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_CREATE) ==
        CMA_LEDGER_ERROR_INSERTION_ERROR);

    asset_id += 2;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND) ==
        CMA_LEDGER_SUCCESS);
    assert(asset_id == 0);

    asset_id += 2;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND_OR_CREATE) ==
        CMA_LEDGER_ERROR_INSERTION_ERROR);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);
    assert(unlink(temp_filepath) == 0);
    printf("%s passed\n", __FUNCTION__);
}
void test_asset_with_token_address(void) {
    char temp_filepath[TMPFILE_PATH_SIZE] = "/tmp/tmpXXXXXX";
    assert(create_temp_file(temp_filepath, FILE_SIZE) == 0);
    cma_ledger_t ledger;
    assert(cma_ledger_init_file(&ledger, temp_filepath, CMA_LEDGER_CREATE_ONLY, 0, FILE_SIZE, MAX_ACCOUNTS, MAX_ASSETS,
               MAX_BALANCES) == CMA_LEDGER_SUCCESS);

    cma_ledger_asset_type_t asset_type = CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS;
    assert(cma_ledger_retrieve_asset(&ledger, NULL, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND) == -EINVAL);

    // clang-format off
    cma_token_address_t token_address1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    }};
    // clang-format on

    assert(cma_ledger_retrieve_asset(&ledger, NULL, &token_address1, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND) ==
        CMA_LEDGER_ERROR_ASSET_NOT_FOUND);

    cma_ledger_asset_id_t asset_id;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, &token_address1, NULL, NULL, &asset_type,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);

    assert(asset_id == 0);

    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, &token_address1, NULL, NULL, &asset_type,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_ERROR_INSERTION_ERROR);

    // clang-format off
    cma_token_address_t token_address2 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    }};
    // clang-format on

    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, &token_address2, NULL, NULL, &asset_type,
               CMA_LEDGER_OP_FIND_OR_CREATE) == CMA_LEDGER_SUCCESS);

    assert(asset_id == 1);

    cma_ledger_asset_id_t asset_id_find;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id_find, &token_address1, NULL, NULL, &asset_type,
               CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);

    assert(asset_id_find == 0);

    cma_ledger_asset_id_t asset_id_to_find = 0;
    cma_token_address_t token_address_find;
    cma_ledger_asset_type_t asset_type_found = CMA_LEDGER_ASSET_TYPE_ID;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id_to_find, &token_address_find, NULL, NULL, &asset_type_found,
               CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);

    assert(asset_type_found == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS);
    assert(memcmp(token_address_find.data, token_address1.data, CMA_ABI_ADDRESS_LENGTH) == 0);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);
    assert(unlink(temp_filepath) == 0);
    printf("%s passed\n", __FUNCTION__);
}

void test_asset_with_token_address_and_id(void) {
    char temp_filepath[TMPFILE_PATH_SIZE] = "/tmp/tmpXXXXXX";
    assert(create_temp_file(temp_filepath, FILE_SIZE) == 0);
    cma_ledger_t ledger;
    assert(cma_ledger_init_file(&ledger, temp_filepath, CMA_LEDGER_CREATE_ONLY, 0, FILE_SIZE, MAX_ACCOUNTS, MAX_ASSETS,
               MAX_BALANCES) == CMA_LEDGER_SUCCESS);

    cma_ledger_asset_type_t asset_type = CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID;
    assert(cma_ledger_retrieve_asset(&ledger, NULL, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND) == -EINVAL);

    // clang-format off
    cma_token_address_t token_address1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    }};
    // clang-format on

    assert(cma_ledger_retrieve_asset(&ledger, NULL, &token_address1, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND) ==
        -EINVAL);

    // clang-format off
    cma_token_id_t token_id1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01,
    }};
    // clang-format on

    assert(cma_ledger_retrieve_asset(&ledger, NULL, &token_address1, &token_id1, NULL, &asset_type,
               CMA_LEDGER_OP_FIND) == CMA_LEDGER_ERROR_ASSET_NOT_FOUND);

    cma_ledger_asset_id_t asset_id;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, &token_address1, &token_id1, NULL, &asset_type,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);

    assert(asset_id == 0);

    cma_ledger_account_id_t account_id;
    cma_ledger_account_type_t account_type = CMA_LEDGER_ACCOUNT_TYPE_ID;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, NULL, &account_type,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);

    // clang-format off
    cma_amount_t amount2 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
        0x00, 0x04,
    }};
    // clang-format on;

    assert(cma_ledger_deposit(&ledger, asset_id, account_id, &amount2) ==
        CMA_LEDGER_ERROR_SUPPLY_OVERFLOW);

    // clang-format off
    cma_amount_t amount = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01,
    }};
    // clang-format on;

    assert(cma_ledger_deposit(&ledger, asset_id, account_id, &amount) ==
        CMA_LEDGER_SUCCESS);
    assert(cma_ledger_deposit(&ledger, asset_id, account_id, &amount) ==
        CMA_LEDGER_ERROR_SUPPLY_OVERFLOW);

    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, &token_address1, &token_id1, NULL, &asset_type,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_ERROR_INSERTION_ERROR);

    // clang-format off
    cma_token_id_t token_id2 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x02,
    }};
    // clang-format on

    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, &token_address1, &token_id2, NULL, &asset_type,
               CMA_LEDGER_OP_FIND_OR_CREATE) == CMA_LEDGER_SUCCESS);

    assert(asset_id == 1);

    cma_ledger_asset_id_t asset_id_find;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id_find, &token_address1, &token_id1, NULL, &asset_type,
               CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);

    assert(asset_id_find == 0);

    cma_ledger_asset_id_t asset_id_to_find = 0;
    cma_token_address_t token_address_find;
    cma_token_id_t token_id_find;
    cma_ledger_asset_type_t asset_type_found = CMA_LEDGER_ASSET_TYPE_ID;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id_to_find, &token_address_find, &token_id_find, NULL,
               &asset_type_found, CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);

    assert(asset_type_found == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID);
    assert(memcmp(token_address_find.data, token_address1.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp(token_id_find.data, token_id1.data, CMA_ABI_ADDRESS_LENGTH) == 0);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);
    assert(unlink(temp_filepath) == 0);
    printf("%s passed\n", __FUNCTION__);
}

void test_asset_with_token_address_and_id_amount(void) {
    char temp_filepath[TMPFILE_PATH_SIZE] = "/tmp/tmpXXXXXX";
    assert(create_temp_file(temp_filepath, FILE_SIZE) == 0);
    cma_ledger_t ledger;
    assert(cma_ledger_init_file(&ledger, temp_filepath, CMA_LEDGER_CREATE_ONLY, 0, FILE_SIZE, MAX_ACCOUNTS, MAX_ASSETS,
               MAX_BALANCES) == CMA_LEDGER_SUCCESS);

    cma_ledger_asset_type_t asset_type = CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID_AMOUNT;
    assert(cma_ledger_retrieve_asset(&ledger, NULL, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND) == -EINVAL);

    // clang-format off
    cma_token_address_t token_address1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    }};
    // clang-format on

    assert(cma_ledger_retrieve_asset(&ledger, NULL, &token_address1, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND) ==
        -EINVAL);

    // clang-format off
    cma_token_id_t token_id1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01,
    }};
    // clang-format on

    assert(cma_ledger_retrieve_asset(&ledger, NULL, &token_address1, &token_id1, NULL, &asset_type,
               CMA_LEDGER_OP_FIND) == CMA_LEDGER_ERROR_ASSET_NOT_FOUND);

    cma_ledger_asset_id_t asset_id;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, &token_address1, &token_id1, NULL, &asset_type,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);

    assert(asset_id == 0);

    cma_ledger_account_id_t account_id;
    cma_ledger_account_type_t account_type = CMA_LEDGER_ACCOUNT_TYPE_ID;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, NULL, &account_type,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);

    // clang-format off
    cma_amount_t amount = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
        0x00, 0x04,
    }};
    // clang-format on;

    assert(cma_ledger_deposit(&ledger, asset_id, account_id, &amount) ==
        CMA_LEDGER_SUCCESS);

    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, &token_address1, &token_id1, NULL, &asset_type,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_ERROR_INSERTION_ERROR);

    // clang-format off
    cma_token_id_t token_id2 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x02,
    }};
    // clang-format on

    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, &token_address1, &token_id2, NULL, &asset_type,
               CMA_LEDGER_OP_FIND_OR_CREATE) == CMA_LEDGER_SUCCESS);

    assert(asset_id == 1);

    cma_ledger_asset_id_t asset_id_find;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id_find, &token_address1, &token_id1, NULL, &asset_type,
               CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);

    assert(asset_id_find == 0);

    cma_ledger_asset_id_t asset_id_to_find = 0;
    cma_token_address_t token_address_find;
    cma_token_id_t token_id_find;
    cma_ledger_asset_type_t asset_type_found = CMA_LEDGER_ASSET_TYPE_ID;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id_to_find, &token_address_find, &token_id_find, NULL,
               &asset_type_found, CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);

    assert(asset_type_found == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID_AMOUNT);
    assert(memcmp(token_address_find.data, token_address1.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp(token_id_find.data, token_id1.data, CMA_ABI_ADDRESS_LENGTH) == 0);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);
    assert(unlink(temp_filepath) == 0);
    printf("%s passed\n", __FUNCTION__);
}

void test_account_id(void) {
    char temp_filepath[TMPFILE_PATH_SIZE] = "/tmp/tmpXXXXXX";
    assert(create_temp_file(temp_filepath, FILE_SIZE) == 0);
    cma_ledger_t ledger;
    assert(cma_ledger_init_file(&ledger, temp_filepath, CMA_LEDGER_CREATE_ONLY, 0, FILE_SIZE, MAX_ACCOUNTS, MAX_ASSETS,
               MAX_BALANCES) == CMA_LEDGER_SUCCESS);

    cma_ledger_account_type_t account_type = CMA_LEDGER_ACCOUNT_TYPE_ID;
    assert(cma_ledger_retrieve_account(&ledger, NULL, NULL, NULL, NULL, &account_type, CMA_LEDGER_OP_FIND) == -EINVAL);

    cma_ledger_account_id_t account_id = 0;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, NULL, &account_type, CMA_LEDGER_OP_FIND) ==
        CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);

    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, NULL, &account_type, CMA_LEDGER_OP_CREATE) ==
        CMA_LEDGER_SUCCESS);

    assert(account_id == 0);

    account_id += 2;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, NULL, &account_type,
               CMA_LEDGER_OP_FIND_OR_CREATE) == CMA_LEDGER_SUCCESS);

    assert(account_id == 1);

    account_id = 0;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, NULL, &account_type, CMA_LEDGER_OP_FIND) ==
        CMA_LEDGER_SUCCESS);

    account_id = 0;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, NULL, &account_type,
               CMA_LEDGER_OP_FIND_OR_CREATE) == CMA_LEDGER_SUCCESS);
    assert(account_id == 0);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);
    assert(unlink(temp_filepath) == 0);
    printf("%s passed\n", __FUNCTION__);
}

void test_account_address(void) {
    char temp_filepath[TMPFILE_PATH_SIZE] = "/tmp/tmpXXXXXX";
    assert(create_temp_file(temp_filepath, FILE_SIZE) == 0);
    cma_ledger_t ledger;
    assert(cma_ledger_init_file(&ledger, temp_filepath, CMA_LEDGER_CREATE_ONLY, 0, FILE_SIZE, MAX_ACCOUNTS, MAX_ASSETS,
               MAX_BALANCES) == CMA_LEDGER_SUCCESS);

    cma_ledger_account_type_t account_type = CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS;
    assert(cma_ledger_retrieve_account(&ledger, NULL, NULL, NULL, NULL, &account_type, CMA_LEDGER_OP_FIND) == -EINVAL);

    // clang-format off
    cma_ledger_account_t account1 = {.address = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    }}};
    // clang-format on;
    assert(cma_ledger_retrieve_account(&ledger, NULL, &account1, NULL, NULL, &account_type,
                CMA_LEDGER_OP_FIND) == CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);

    cma_ledger_account_id_t account_id;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, &account1, NULL, NULL, &account_type,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);
    assert(account_id == 0);

    assert(cma_ledger_retrieve_account(&ledger, &account_id, &account1, NULL, NULL, &account_type,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_ERROR_INSERTION_ERROR);

    // clang-format off
    cma_ledger_account_t account2 = {.address = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    }}};
    // clang-format on;

    assert(cma_ledger_retrieve_account(&ledger, &account_id, &account2, NULL, NULL, &account_type,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);
    assert(account_id == 1);

    cma_ledger_account_id_t account_id_found;
    assert(cma_ledger_retrieve_account(&ledger, &account_id_found, &account1, NULL,
        NULL, &account_type, CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);
    assert(account_id_found == 0);

    cma_ledger_account_id_t account_id_to_find = 0;
    cma_ledger_account_t account_found;
    cma_ledger_account_type_t account_type_found = CMA_LEDGER_ACCOUNT_TYPE_ID;
    assert(cma_ledger_retrieve_account(&ledger, &account_id_to_find, &account_found, NULL, NULL, &account_type_found,
               CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);
    assert(account_type_found == CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS);
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
        NULL, &account_type, CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);
    assert(account_id_found == 0);

    cma_ledger_account_t account_found2 = {};
    assert(cma_ledger_retrieve_account(&ledger, &account_id_to_find, &account_found2, NULL,
        NULL, &account_type, CMA_LEDGER_OP_FIND) == CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);

    // clang-format off
    cma_abi_address_t address2 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    }};
    // clang-format on;

    assert(cma_ledger_retrieve_account(&ledger, NULL, NULL, &address2, NULL, &account_type,
               CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_retrieve_account(&ledger, &account_id_found, NULL, &address2,
        NULL, &account_type, CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);
    assert(account_id_found == 1);

    assert(cma_ledger_retrieve_account(&ledger, NULL, &account_found2, &address2, NULL, &account_type,
               CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);
    assert(memcmp(account_found2.address.data, account2.address.data, CMA_ABI_ADDRESS_LENGTH) == 0);

    assert(cma_ledger_retrieve_account(&ledger, &account_id_found, &account_found2, &address2,
        NULL, &account_type, CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS); assert(account_id_found == 1);
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
        NULL, &account_type, CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);
    assert(account_id_created == 2);
    assert(memcmp(account_created.address.data, address3.data, CMA_ABI_ADDRESS_LENGTH) == 0);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);
    assert(unlink(temp_filepath) == 0);
    printf("%s passed\n", __FUNCTION__);
}

void test_account_full_id(void) {
    char temp_filepath[TMPFILE_PATH_SIZE] = "/tmp/tmpXXXXXX";
    assert(create_temp_file(temp_filepath,FILE_SIZE) == 0);
    cma_ledger_t ledger;
    assert(cma_ledger_init_file(&ledger,temp_filepath,CMA_LEDGER_CREATE_ONLY,0,FILE_SIZE,MAX_ACCOUNTS,MAX_ASSETS,MAX_BALANCES) == CMA_LEDGER_SUCCESS);

    cma_ledger_account_type_t account_type = CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID;
    assert(cma_ledger_retrieve_account(&ledger, NULL, NULL, NULL, NULL, &account_type,
               CMA_LEDGER_OP_FIND) == -EINVAL);

    // clang-format off
    cma_ledger_account_t account1 = {.account_id = {.data = {
        0xde, 0xad, 0xbe, 0xef, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01,
    }}};
    // clang-format on;
    assert(cma_ledger_retrieve_account(&ledger, NULL, &account1, NULL, NULL, &account_type,
                CMA_LEDGER_OP_FIND) == CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);

    cma_ledger_account_id_t account_id;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, &account1, NULL, NULL, &account_type,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);
    assert(account_id == 0);

    assert(cma_ledger_retrieve_account(&ledger, &account_id, &account1, NULL, NULL, &account_type,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_ERROR_INSERTION_ERROR);

    // clang-format off
    cma_ledger_account_t account2 = {.account_id = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x02,
    }}};
    // clang-format on;

    assert(cma_ledger_retrieve_account(&ledger, &account_id, &account2, NULL, NULL, &account_type,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);
    assert(account_id == 1);

    cma_ledger_account_id_t account_id_found;
    assert(cma_ledger_retrieve_account(&ledger, &account_id_found, &account1, NULL, NULL, &account_type,
               CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);
    assert(account_id_found == 0);

    cma_ledger_account_id_t account_id_to_find = 0;
    cma_ledger_account_t account_found;
    cma_ledger_account_type_t account_type_found = CMA_LEDGER_ACCOUNT_TYPE_ID;
    assert(cma_ledger_retrieve_account(&ledger, &account_id_to_find, &account_found, NULL, NULL, &account_type_found,
               CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);
    assert(account_type_found == CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID);
    assert(memcmp(account_found.address.data, account1.address.data, CMA_ABI_ADDRESS_LENGTH) == 0);

    // clang-format off
    cma_ledger_account_t account_to_find = {.address = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    }}};
    // clang-format on;

    assert(cma_ledger_retrieve_account(&ledger, &account_id_found, &account_to_find, NULL,
        NULL, &account_type, CMA_LEDGER_OP_FIND) == CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);

    // clang-format off
    cma_ledger_account_t account_to_find2 = {.address = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    }}};
    // clang-format on;

    // full id 2 is has initial 10 bytes zeroed
    assert(cma_ledger_retrieve_account(&ledger, &account_id_found, &account_to_find2, NULL,
NULL, &account_type, CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS); assert(account_id_found == 1);

    cma_ledger_account_t account_found2 = {};
    assert(cma_ledger_retrieve_account(&ledger, &account_id_to_find, &account_found2, NULL,
NULL, &account_type, CMA_LEDGER_OP_FIND) == CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);

    // clang-format off
    cma_account_id_t full_account2 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x02,
    }};
    // clang-format on;

    assert(cma_ledger_retrieve_account(&ledger, NULL, NULL, &full_account2, NULL, &account_type,
               CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_retrieve_account(&ledger, &account_id_found, NULL, &full_account2,
        NULL, &account_type, CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);
    assert(account_id_found == 1);

    assert(cma_ledger_retrieve_account(&ledger, NULL, &account_found2, &full_account2,
        NULL, &account_type, CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);
    assert(memcmp(account_found2.account_id.data, account2.account_id.data, CMA_ABI_ADDRESS_LENGTH) == 0);

    assert(cma_ledger_retrieve_account(&ledger, &account_id_found, &account_found2, &full_account2,
        NULL, &account_type, CMA_LEDGER_OP_FIND) == CMA_LEDGER_SUCCESS);
    assert(account_id_found == 1);
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
        NULL, &account_type, CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);
    assert(account_id_created == 2);
    assert(memcmp(account_created.account_id.data, full_account3.data, CMA_ABI_ADDRESS_LENGTH) == 0);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);
    assert(unlink(temp_filepath) == 0);
    printf("%s passed\n", __FUNCTION__);
}

void test_remove(void) {
    char temp_filepath[TMPFILE_PATH_SIZE] = "/tmp/tmpXXXXXX";
    assert(create_temp_file(temp_filepath,FILE_SIZE) == 0);
    cma_ledger_t ledger;
    assert(cma_ledger_init_file(&ledger,temp_filepath,CMA_LEDGER_CREATE_ONLY,0,FILE_SIZE,MAX_ACCOUNTS,MAX_ASSETS,MAX_BALANCES) == CMA_LEDGER_SUCCESS);

    cma_ledger_asset_id_t asset_id;
    cma_ledger_asset_type_t asset_type = CMA_LEDGER_ASSET_TYPE_ID;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND_AND_REMOVE) ==
        CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_CREATE) ==
        CMA_LEDGER_SUCCESS);
    assert(asset_id == 0);

    cma_ledger_account_id_t account_id;
    cma_ledger_account_type_t account_type = CMA_LEDGER_ACCOUNT_TYPE_ID;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, NULL, &account_type,
        CMA_LEDGER_OP_FIND_AND_REMOVE) == CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);
    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, NULL, &account_type,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);
    assert(account_id == 0);

    // clang-format off
    cma_amount_t amount = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
        0x00, 0x04,
    }};
    // clang-format on;

    assert(cma_ledger_deposit(&ledger, asset_id, account_id, &amount) ==
        CMA_LEDGER_SUCCESS);

    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND_AND_REMOVE) ==
        CMA_LEDGER_ERROR_ASSET_SUPPLY);
    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, NULL, &account_type,
        CMA_LEDGER_OP_FIND_AND_REMOVE) == CMA_LEDGER_ERROR_ACCOUNT_BALANCE);

    assert(cma_ledger_withdraw(&ledger, asset_id, account_id, &amount) ==
        CMA_LEDGER_SUCCESS);

    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND_AND_REMOVE) ==
        CMA_LEDGER_SUCCESS);
    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, NULL, &account_type,
        CMA_LEDGER_OP_FIND_AND_REMOVE) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND_AND_REMOVE) ==
        CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, NULL, &account_type,
        CMA_LEDGER_OP_FIND_AND_REMOVE) == CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);

    // clang-format off
    cma_token_address_t token_address1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    }};
    // clang-format on
    asset_type = CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, &token_address1, NULL, NULL, &asset_type,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);
    assert(asset_id == 1);

    // clang-format off
    cma_ledger_account_t account1 = {.address = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    }}};
    // clang-format on;
    account_type = CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, &account1, NULL, NULL, &account_type,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);
    assert(account_id == 1);

    assert(cma_ledger_deposit(&ledger, asset_id, account_id, &amount) ==
        CMA_LEDGER_SUCCESS);

    assert(cma_ledger_retrieve_asset(&ledger, NULL, &token_address1, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND_AND_REMOVE) ==
        CMA_LEDGER_ERROR_ASSET_SUPPLY);
    assert(cma_ledger_retrieve_account(&ledger, NULL, &account1, NULL, NULL, &account_type,
        CMA_LEDGER_OP_FIND_AND_REMOVE) == CMA_LEDGER_ERROR_ACCOUNT_BALANCE);

    assert(cma_ledger_withdraw(&ledger, asset_id, account_id, &amount) ==
        CMA_LEDGER_SUCCESS);

    assert(cma_ledger_retrieve_asset(&ledger, NULL, &token_address1, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND_AND_REMOVE) ==
        CMA_LEDGER_SUCCESS);
    assert(cma_ledger_retrieve_account(&ledger, NULL, &account1, NULL, NULL, &account_type,
        CMA_LEDGER_OP_FIND_AND_REMOVE) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_retrieve_asset(&ledger, NULL, &token_address1, NULL, NULL, &asset_type, CMA_LEDGER_OP_FIND_AND_REMOVE) ==
        CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
    assert(cma_ledger_retrieve_account(&ledger, NULL, &account1, NULL, NULL, &account_type,
        CMA_LEDGER_OP_FIND_AND_REMOVE) == CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);

    // clang-format off
    cma_token_id_t token_id1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01,
    }};
    // clang-format on
    asset_type = CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID_AMOUNT;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, &token_address1, &token_id1, NULL, &asset_type,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);
    assert(asset_id == 2);

    // clang-format off
    cma_ledger_account_t account2 = {.account_id = {.data = {
        0xde, 0xad, 0xbe, 0xef, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01,
    }}};
    // clang-format on;
    account_type = CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, &account2, NULL, NULL, &account_type,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);
    assert(account_id == 2);

    assert(cma_ledger_deposit(&ledger, asset_id, account_id, &amount) ==
        CMA_LEDGER_SUCCESS);

    assert(cma_ledger_retrieve_asset(&ledger, NULL,  &token_address1, &token_id1, NULL, &asset_type, CMA_LEDGER_OP_FIND_AND_REMOVE) ==
        CMA_LEDGER_ERROR_ASSET_SUPPLY);
    assert(cma_ledger_retrieve_account(&ledger, NULL, &account2, NULL, NULL, &account_type,
        CMA_LEDGER_OP_FIND_AND_REMOVE) == CMA_LEDGER_ERROR_ACCOUNT_BALANCE);

    assert(cma_ledger_withdraw(&ledger, asset_id, account_id, &amount) ==
        CMA_LEDGER_SUCCESS);

    assert(cma_ledger_retrieve_asset(&ledger, NULL,  &token_address1, &token_id1, NULL, &asset_type, CMA_LEDGER_OP_FIND_AND_REMOVE) ==
        CMA_LEDGER_SUCCESS);
    assert(cma_ledger_retrieve_account(&ledger, NULL, &account2, NULL, NULL, &account_type,
        CMA_LEDGER_OP_FIND_AND_REMOVE) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_retrieve_asset(&ledger, NULL,  &token_address1, &token_id1, NULL, &asset_type, CMA_LEDGER_OP_FIND_AND_REMOVE) ==
        CMA_LEDGER_ERROR_ASSET_NOT_FOUND);
    assert(cma_ledger_retrieve_account(&ledger, NULL, &account2, NULL, NULL, &account_type,
        CMA_LEDGER_OP_FIND_AND_REMOVE) == CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);

    assert(unlink(temp_filepath) == 0);
    printf("%s passed\n", __FUNCTION__);
}

void test_balance_mem(void) {
    const size_t file_size = CMA_LEDGER_MIN_MEM_LENGTH + 4340;
    char temp_filepath[TMPFILE_PATH_SIZE] = "/tmp/tmpXXXXXX";
    assert(create_temp_file(temp_filepath,file_size) == 0);
    cma_ledger_t ledger;

    assert(cma_ledger_init_file(&ledger,temp_filepath,CMA_LEDGER_CREATE_ONLY,0,CMA_LEDGER_MIN_MEM_LENGTH+4340,10,1,10) == CMA_LEDGER_SUCCESS);

    uint8_t *buffer = malloc(file_size);
    assert(buffer != NULL);

    // clang-format off
    cma_token_address_t token_address1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    }};
    // clang-format on
    cma_ledger_asset_id_t asset_id;
    cma_ledger_asset_type_t asset_type = CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, &token_address1, NULL, NULL, &asset_type,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);
    assert(asset_id == 0);

    // clang-format off
    cma_ledger_account_t account1 = {.address = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    }}};
    // clang-format on;
    cma_ledger_account_id_t account_id;
    cma_ledger_account_type_t account_type = CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, &account1, NULL, NULL, &account_type,
                CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);
    assert(account_id == 0);

    // clang-format off
    cma_amount_t amount = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
        0x00, 0x04,
    }};
    // clang-format on;

    assert(cma_ledger_deposit(&ledger, asset_id, account_id, &amount) ==
        CMA_LEDGER_SUCCESS);

    cma_ledger_account_balance_info_t account_balance_info = {};
    assert(cma_ledger_get_balance(&ledger, 1000, 1000, NULL, &account_balance_info) ==
        CMA_LEDGER_SUCCESS);
    assert(account_balance_info.balance == NULL);

    assert(cma_ledger_get_balance(&ledger, asset_id, account_id, NULL, &account_balance_info) ==
        CMA_LEDGER_SUCCESS);

    // clang-format off
    uint8_t zero[CMA_ABI_U256_LENGTH] =  {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00,
    };
    // clang-format on;

    assert(account_balance_info.balance->type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS);
    assert(memcmp(account_balance_info.balance->owner.data, account1.address.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp(account_balance_info.balance->token_address.data, token_address1.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp(account_balance_info.balance->token_id.data, zero, CMA_ABI_ID_LENGTH) == 0);
    assert(memcmp(account_balance_info.balance->amount.data, amount.data, CMA_ABI_U256_LENGTH) == 0);

    FILE *file = fopen(temp_filepath, "rb");
    assert(file != NULL);
    assert(fread(buffer, sizeof(uint8_t), file_size, file) == file_size);
    fclose(file);

    cma_ledger_account_balance_t *account_balance = (cma_ledger_account_balance_t *)(buffer + account_balance_info.offset +
        sizeof(cma_ledger_account_balance_t) * account_balance_info.index);

    assert(account_balance->type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS);
    assert(memcmp(account_balance->owner.data, account1.address.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp(account_balance->token_address.data, token_address1.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp(account_balance->token_id.data, zero, CMA_ABI_ID_LENGTH) == 0);
    assert(memcmp(account_balance->amount.data, amount.data, CMA_ABI_U256_LENGTH) == 0);

    assert(cma_ledger_withdraw(&ledger, asset_id, account_id, &amount) ==
        CMA_LEDGER_SUCCESS);

    file = fopen(temp_filepath, "rb");
    assert(file != NULL);
    assert(fread(buffer, sizeof(uint8_t), file_size, file) == file_size);
    fclose(file);

    assert(account_balance->type == 0);
    assert(memcmp(account_balance->owner.data, zero, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp(account_balance->token_address.data, zero, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp(account_balance->token_id.data, zero, CMA_ABI_ID_LENGTH) == 0);
    assert(memcmp(account_balance->amount.data, zero, CMA_ABI_U256_LENGTH) == 0);

    cma_ledger_account_id_t account_id_tmp;
    // clang-format off
    cma_ledger_account_t account2 = {.address = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    }}};
    cma_ledger_account_t account3 = {.address = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
    }}};
    // clang-format on;

    assert(cma_ledger_deposit(&ledger, asset_id, account_id, &amount) ==
        CMA_LEDGER_SUCCESS);

    assert(cma_ledger_retrieve_account(&ledger, &account_id_tmp, &account2, NULL, NULL, &account_type,
                CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);
    assert(cma_ledger_deposit(&ledger, asset_id, account_id_tmp, &amount) ==
        CMA_LEDGER_SUCCESS);
    assert(cma_ledger_retrieve_account(&ledger, &account_id_tmp, &account3, NULL, NULL, &account_type,
                CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);
    assert(cma_ledger_deposit(&ledger, asset_id, account_id_tmp, &amount) ==
        CMA_LEDGER_SUCCESS);

    file = fopen(temp_filepath, "rb");
    assert(file != NULL);
    assert(fread(buffer, sizeof(uint8_t), file_size, file) == file_size);
    fclose(file);

    assert(account_balance->type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS);
    assert(memcmp(account_balance->owner.data, account1.address.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp(account_balance->token_address.data, token_address1.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp(account_balance->token_id.data, zero, CMA_ABI_ID_LENGTH) == 0);
    assert(memcmp(account_balance->amount.data, amount.data, CMA_ABI_U256_LENGTH) == 0);

    assert((account_balance+1)->type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS);
    assert(memcmp((account_balance+1)->owner.data, account2.address.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp((account_balance+1)->token_address.data, token_address1.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp((account_balance+1)->token_id.data, zero, CMA_ABI_ID_LENGTH) == 0);
    assert(memcmp((account_balance+1)->amount.data, amount.data, CMA_ABI_U256_LENGTH) == 0);

    assert((account_balance+2)->type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS);
    assert(memcmp((account_balance+2)->owner.data, account3.address.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp((account_balance+2)->token_address.data, token_address1.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp((account_balance+2)->token_id.data, zero, CMA_ABI_ID_LENGTH) == 0);
    assert(memcmp((account_balance+2)->amount.data, amount.data, CMA_ABI_U256_LENGTH) == 0);

    assert(cma_ledger_withdraw(&ledger, asset_id, account_id, &amount) ==
        CMA_LEDGER_SUCCESS);

    file = fopen(temp_filepath, "rb");
    assert(file != NULL);
    assert(fread(buffer, sizeof(uint8_t), file_size, file) == file_size);
    fclose(file);

    assert(memcmp(account_balance->owner.data, account3.address.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp((account_balance+1)->owner.data, account2.address.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp((account_balance+2)->owner.data, zero, CMA_ABI_ADDRESS_LENGTH) == 0);

    assert(cma_ledger_get_balance(&ledger, asset_id, account_id_tmp, NULL, &account_balance_info) ==
        CMA_LEDGER_SUCCESS);
    assert(account_balance_info.index == 0);

    // printf("file %s\n", temp_filepath);
    free(buffer);
    buffer = NULL;
    assert(unlink(temp_filepath) == 0);
    printf("%s passed\n", __FUNCTION__);
}

void test_asset_total_supply_balance(void) {
    char temp_filepath[TMPFILE_PATH_SIZE] = "/tmp/tmpXXXXXX";
    assert(create_temp_file(temp_filepath,FILE_SIZE) == 0);
    cma_ledger_t ledger;
    assert(cma_ledger_init_file(&ledger,temp_filepath,CMA_LEDGER_CREATE_ONLY,0,FILE_SIZE,MAX_ACCOUNTS,MAX_ASSETS,MAX_BALANCES) == CMA_LEDGER_SUCCESS);

    cma_ledger_asset_id_t asset_id;
    cma_ledger_asset_type_t asset_type = CMA_LEDGER_ASSET_TYPE_BASE;
    cma_amount_t out_total_supply = {};
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, &out_total_supply, &asset_type, CMA_LEDGER_OP_CREATE) ==
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
    assert(cma_ledger_get_balance(&ledger, 1000, 1000, &balance, NULL) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(balance.data, zero_amount.data, CMA_ABI_U256_LENGTH) == 0);

    assert(cma_ledger_get_balance(&ledger, asset_id, 1000, &balance, NULL) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(balance.data, zero_amount.data, CMA_ABI_U256_LENGTH) == 0);

    cma_ledger_account_id_t account_id;
    cma_ledger_account_type_t account_type = CMA_LEDGER_ACCOUNT_TYPE_ID;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, NULL, &account_type,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_get_balance(&ledger, asset_id, account_id, &balance, NULL) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(balance.data, zero_amount.data, CMA_ABI_U256_LENGTH) == 0);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);
    assert(unlink(temp_filepath) == 0);
    printf("%s passed\n", __FUNCTION__);
}

void test_deposit(void) {
    char temp_filepath[TMPFILE_PATH_SIZE] = "/tmp/tmpXXXXXX";
    assert(create_temp_file(temp_filepath,FILE_SIZE) == 0);
    cma_ledger_t ledger;
    assert(cma_ledger_init_file(&ledger,temp_filepath,CMA_LEDGER_CREATE_ONLY,0,FILE_SIZE,MAX_ACCOUNTS,MAX_ASSETS,MAX_BALANCES) == CMA_LEDGER_SUCCESS);

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
    cma_ledger_asset_type_t asset_type = CMA_LEDGER_ASSET_TYPE_BASE;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_CREATE) ==
        CMA_LEDGER_SUCCESS);

    assert(cma_ledger_deposit(&ledger, asset_id, 1000, &amount) ==
        CMA_LEDGER_ERROR_ACCOUNT_NOT_FOUND);

    cma_ledger_account_id_t account_id;
    cma_ledger_account_type_t account_type = CMA_LEDGER_ACCOUNT_TYPE_ID;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, NULL, &account_type,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_deposit(&ledger, 1000, account_id, &amount) ==
        CMA_LEDGER_ERROR_ASSET_NOT_FOUND);

    assert(cma_ledger_deposit(&ledger, asset_id, account_id, &amount) ==
        CMA_LEDGER_SUCCESS);

    cma_amount_t supply = {};
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, &supply, &asset_type, CMA_LEDGER_OP_FIND) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(supply.data, amount.data, CMA_ABI_U256_LENGTH) == 0);

    cma_amount_t balance = {};
    assert(cma_ledger_get_balance(&ledger, asset_id, account_id, &balance, NULL) ==
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

    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, &supply, &asset_type, CMA_LEDGER_OP_FIND) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(supply.data, amount2.data, CMA_ABI_U256_LENGTH) == 0);

    assert(cma_ledger_get_balance(&ledger, asset_id, account_id, &balance, NULL) ==
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
    assert(cma_ledger_retrieve_account(&ledger, &account_id2, NULL, NULL, NULL, &account_type,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_deposit(&ledger, asset_id, account_id2, &amount) ==
        CMA_LEDGER_SUCCESS);
    assert(cma_ledger_deposit(&ledger, asset_id, account_id2, &amount) ==
        CMA_LEDGER_SUCCESS);

    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, &supply, &asset_type, CMA_LEDGER_OP_FIND) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(supply.data, amount4.data, CMA_ABI_U256_LENGTH) == 0);

    assert(cma_ledger_get_balance(&ledger, asset_id, account_id, &balance, NULL) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(balance.data, amount2.data, CMA_ABI_U256_LENGTH) == 0);

    assert(cma_ledger_get_balance(&ledger, asset_id, account_id2, &balance, NULL) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(balance.data, amount2.data, CMA_ABI_U256_LENGTH) == 0);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);
    assert(unlink(temp_filepath) == 0);
    printf("%s passed\n", __FUNCTION__);
}

void test_withdraw(void) {
    char temp_filepath[TMPFILE_PATH_SIZE] = "/tmp/tmpXXXXXX";
    assert(create_temp_file(temp_filepath,FILE_SIZE) == 0);
    cma_ledger_t ledger;
    assert(cma_ledger_init_file(&ledger,temp_filepath,CMA_LEDGER_CREATE_ONLY,0,FILE_SIZE,MAX_ACCOUNTS,MAX_ASSETS,MAX_BALANCES) == CMA_LEDGER_SUCCESS);

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
    cma_ledger_asset_type_t asset_type = CMA_LEDGER_ASSET_TYPE_BASE;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_CREATE) ==
        CMA_LEDGER_SUCCESS);

    assert(cma_ledger_withdraw(&ledger, asset_id, 1000, &amount) ==
        CMA_LEDGER_ERROR_SUPPLY_OVERFLOW);

    cma_ledger_account_id_t account_id;
    cma_ledger_account_type_t account_type = CMA_LEDGER_ACCOUNT_TYPE_ID;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, NULL, &account_type,
               CMA_LEDGER_OP_CREATE) == CMA_LEDGER_SUCCESS);

    assert(cma_ledger_withdraw(&ledger, 1000, account_id, &amount) ==
        CMA_LEDGER_ERROR_ASSET_NOT_FOUND);

    assert(cma_ledger_deposit(&ledger, asset_id, account_id, &amount) ==
        CMA_LEDGER_SUCCESS);

    cma_ledger_account_id_t account_id2;
    assert(cma_ledger_retrieve_account(&ledger, &account_id2, NULL, NULL, NULL, &account_type,
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
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, &supply, &asset_type, CMA_LEDGER_OP_FIND) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(supply.data, amount2.data, CMA_ABI_U256_LENGTH) == 0);

    cma_amount_t balance = {};
    assert(cma_ledger_get_balance(&ledger, asset_id, account_id, &balance, NULL) ==
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

    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, &supply, &asset_type, CMA_LEDGER_OP_FIND) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(supply.data, amount4.data, CMA_ABI_U256_LENGTH) == 0);

    assert(cma_ledger_get_balance(&ledger, asset_id, account_id, &balance, NULL) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(balance.data, amount2.data, CMA_ABI_U256_LENGTH) == 0);

    assert(cma_ledger_get_balance(&ledger, asset_id, account_id2, &balance, NULL) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(balance.data, amount2.data, CMA_ABI_U256_LENGTH) == 0);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);
    assert(unlink(temp_filepath) == 0);
    printf("%s passed\n", __FUNCTION__);
}

void test_transfer(void) {
    char temp_filepath[TMPFILE_PATH_SIZE] = "/tmp/tmpXXXXXX";
    assert(create_temp_file(temp_filepath,FILE_SIZE) == 0);
    cma_ledger_t ledger;
    assert(cma_ledger_init_file(&ledger,temp_filepath,CMA_LEDGER_CREATE_ONLY,0,FILE_SIZE,MAX_ACCOUNTS,MAX_ASSETS,MAX_BALANCES) == CMA_LEDGER_SUCCESS);

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
    cma_ledger_asset_type_t asset_type = CMA_LEDGER_ASSET_TYPE_BASE;
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, NULL, &asset_type, CMA_LEDGER_OP_CREATE) ==
        CMA_LEDGER_SUCCESS);

    assert(cma_ledger_transfer(&ledger, asset_id, 1000, 1000, &amount) ==
        -EINVAL);

    cma_ledger_account_id_t account_id;
    cma_ledger_account_type_t account_type = CMA_LEDGER_ACCOUNT_TYPE_ID;
    assert(cma_ledger_retrieve_account(&ledger, &account_id, NULL, NULL, NULL, &account_type,
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
    assert(cma_ledger_retrieve_account(&ledger, &account_id2, NULL, NULL, NULL, &account_type,
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
    assert(cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, &supply, &asset_type, CMA_LEDGER_OP_FIND) ==
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
    assert(cma_ledger_get_balance(&ledger, asset_id, account_id, &balance, NULL) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(balance.data, amount.data, CMA_ABI_U256_LENGTH) == 0);
    assert(cma_ledger_get_balance(&ledger, asset_id, account_id2, &balance, NULL) ==
        CMA_LEDGER_SUCCESS);
    assert(memcmp(balance.data, amount3.data, CMA_ABI_U256_LENGTH) == 0);

    assert(cma_ledger_fini(&ledger) == CMA_LEDGER_SUCCESS);
    assert(unlink(temp_filepath) == 0);
    printf("%s passed\n", __FUNCTION__);
}

int main(void) {
    test_init_and_fini();
    test_init_and_reset();
    test_init_and_load();
    test_asset_id();
    test_asset_base();
    test_asset_with_token_address();
    test_asset_with_token_address_and_id();
    test_asset_with_token_address_and_id_amount();
    test_account_id();
    test_account_address();
    test_account_full_id();
    test_asset_total_supply_balance();
    test_deposit();
    test_withdraw();
    test_transfer();
    test_remove();
    test_balance_mem();
    printf("All file-ledger tests passed!\n");
    return 0;
}
