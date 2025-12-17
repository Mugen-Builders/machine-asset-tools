#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <libcmt/rollup.h>

#include "libcma/parser.h"

void test_ether_deposit(void) {
    cma_parser_input_t parser_input;

    // clang-format off
    uint8_t data_advance1[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, // address 20 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x04, // amount 32 bytes
        // data 0 bytes
    };
    cmt_rollup_advance_t advance1 = {.payload = { .length = sizeof(data_advance1), .data = data_advance1 }};
    // clang-format on

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_DEPOSIT, NULL, NULL) == -EINVAL);
    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_DEPOSIT, &advance1, NULL) == -EINVAL);
    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_DEPOSIT, NULL, &parser_input) == -EINVAL);
    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_AUTO, &advance1, &parser_input) == -EINVAL);

    assert(
        cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_DEPOSIT, &advance1, &parser_input) == CMA_PARSER_SUCCESS);

    assert(parser_input.type == CMA_PARSER_INPUT_TYPE_ETHER_DEPOSIT);

    // clang-format off
    cma_token_address_t address1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    }};
    cma_amount_t amount1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x04,
    }};
    // clang-format on

    assert(memcmp(parser_input.ether_deposit.sender.data, address1.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp(parser_input.ether_deposit.amount.data, amount1.data, CMA_ABI_U256_LENGTH) == 0);
    assert(parser_input.ether_deposit.exec_layer_data.length == 0);

    // clang-format off
    uint8_t data_advance2[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, // address 20 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x04, // amount 32 bytes
        0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef // data 8 bytes
    };
    cmt_rollup_advance_t advance2 = {.payload = { .length = sizeof(data_advance2), .data = data_advance2 }};
    // clang-format on

    // clang-format off
    uint8_t data2[] = {
        0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef
    };
    // clang-format on

    assert(
        cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_DEPOSIT, &advance2, &parser_input) == CMA_PARSER_SUCCESS);

    assert(memcmp(parser_input.ether_deposit.sender.data, address1.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp(parser_input.ether_deposit.amount.data, amount1.data, CMA_ABI_U256_LENGTH) == 0);
    assert(parser_input.ether_deposit.exec_layer_data.length == sizeof(data2));
    assert(memcmp(parser_input.ether_deposit.exec_layer_data.data, data2, sizeof(data2)) == 0);

    // clang-format off
    uint8_t malformed_data_advance1[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,  // address 18 bytes
    };
    uint8_t malformed_data_advance2[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, // address 20 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x04, // amount 22 bytes
    };
    // clang-format on

    cmt_rollup_advance_t malformed_advance1 = {
        .payload = {.length = sizeof(malformed_data_advance1), .data = malformed_data_advance1}};
    cmt_rollup_advance_t malformed_advance2 = {
        .payload = {.length = sizeof(malformed_data_advance2), .data = malformed_data_advance2}};
    cmt_rollup_advance_t malformed_advance3 = {.payload = {.length = 10, .data = data_advance1}};
    cmt_rollup_advance_t malformed_advance4 = {
        .payload = {.length = sizeof(data_advance2) + 20, .data = data_advance2}};

    assert(
        cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_DEPOSIT, &malformed_advance1, &parser_input) == -ENOBUFS);

    assert(
        cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_DEPOSIT, &malformed_advance2, &parser_input) == -ENOBUFS);

    assert(
        cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_DEPOSIT, &malformed_advance3, &parser_input) == -ENOBUFS);

    // despite being malformed, the parser doesn't give an error because it can't detect (the error is the definition of
    //   the advance input)
    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_DEPOSIT, &malformed_advance4, &parser_input) ==
        CMA_PARSER_SUCCESS);

    printf("%s passed\n", __FUNCTION__);
}

void test_erc20_deposit(void) {
    cma_parser_input_t parser_input;

    // clang-format off
    uint8_t data_advance1[] = {
        0x01, // success
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, // token 20 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, // address 20 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x04, // amount 32 bytes
        // data 0 bytes
    };
    cmt_rollup_advance_t advance1 = {.payload = { .length = sizeof(data_advance1), .data = data_advance1 }};
    // clang-format on

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_DEPOSIT, NULL, NULL) == -EINVAL);
    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_DEPOSIT, &advance1, NULL) == -EINVAL);
    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_DEPOSIT, NULL, &parser_input) == -EINVAL);
    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_AUTO, &advance1, &parser_input) == -EINVAL);

    assert(
        cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_DEPOSIT, &advance1, &parser_input) == CMA_PARSER_SUCCESS);

    assert(parser_input.type == CMA_PARSER_INPUT_TYPE_ERC20_DEPOSIT);
    assert(parser_input.erc20_deposit.success);

    // clang-format off
    cma_token_address_t address1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    }};
    cma_token_address_t token1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
    }};
    cma_amount_t amount1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x04,
    }};
    // clang-format on

    assert(memcmp(parser_input.erc20_deposit.sender.data, address1.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp(parser_input.erc20_deposit.token.data, token1.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp(parser_input.erc20_deposit.amount.data, amount1.data, CMA_ABI_U256_LENGTH) == 0);
    assert(parser_input.erc20_deposit.exec_layer_data.length == 0);

    // clang-format off
    uint8_t data_advance2[] = {
        0x01, // success
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, // token 20 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, // address 20 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x04, // amount 32 bytes
        0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef // data 8 bytes
    };
    cmt_rollup_advance_t advance2 = {.payload = { .length = sizeof(data_advance2), .data = data_advance2 }};
    // clang-format on

    // clang-format off
    uint8_t data2[] = {
        0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef
    };
    // clang-format on

    assert(
        cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_DEPOSIT, &advance2, &parser_input) == CMA_PARSER_SUCCESS);

    assert(parser_input.erc20_deposit.success);
    assert(memcmp(parser_input.erc20_deposit.sender.data, address1.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp(parser_input.erc20_deposit.amount.data, amount1.data, CMA_ABI_U256_LENGTH) == 0);
    assert(parser_input.erc20_deposit.exec_layer_data.length == sizeof(data2));
    assert(memcmp(parser_input.erc20_deposit.exec_layer_data.data, data2, sizeof(data2)) == 0);

    // clang-format off
    uint8_t malformed_data_advance1[] = {
        0x01, // success
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,  // address 18 bytes
    };
    uint8_t malformed_data_advance2[] = {
        0x01, // success
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, // token 20 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,  // address 18 bytes
    };
    uint8_t malformed_data_advance3[] = {
        0x01, // success
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, // address 20 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, // address 20 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x04, // amount 22 bytes
    };
    uint8_t failed_data_advance[] = {
        0x00, // failure
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, // token 20 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, // address 20 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x04, // amount 32 bytes
        0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef // data 8 bytes
    };
    // clang-format on

    cmt_rollup_advance_t malformed_advance1 = {
        .payload = {.length = sizeof(malformed_data_advance1), .data = malformed_data_advance1}};
    cmt_rollup_advance_t malformed_advance2 = {
        .payload = {.length = sizeof(malformed_data_advance2), .data = malformed_data_advance2}};
    cmt_rollup_advance_t malformed_advance3 = {
        .payload = {.length = sizeof(malformed_data_advance3), .data = malformed_data_advance3}};
    cmt_rollup_advance_t malformed_advance4 = {.payload = {.length = 10, .data = data_advance1}};
    cmt_rollup_advance_t malformed_advance5 = {
        .payload = {.length = sizeof(data_advance2) + 20, .data = data_advance2}};
    cmt_rollup_advance_t failed_advance = {
        .payload = {.length = sizeof(failed_data_advance), .data = failed_data_advance}};

    assert(
        cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_DEPOSIT, &malformed_advance1, &parser_input) == -ENOBUFS);

    assert(
        cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_DEPOSIT, &malformed_advance2, &parser_input) == -ENOBUFS);

    assert(
        cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_DEPOSIT, &malformed_advance3, &parser_input) == -ENOBUFS);

    assert(
        cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_DEPOSIT, &malformed_advance4, &parser_input) == -ENOBUFS);

    // despite being malformed, the parser doesn't give an error because it can't detect (the error is the definition of
    //   the advance input)
    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_DEPOSIT, &malformed_advance5, &parser_input) ==
        CMA_PARSER_SUCCESS);

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_DEPOSIT, &failed_advance, &parser_input) ==
        CMA_PARSER_SUCCESS);
    assert(parser_input.erc20_deposit.success == false);

    printf("%s passed\n", __FUNCTION__);
}

void test_ether_withdraw(void) {
    cma_parser_input_t parser_input;

    // clang-format off
    uint8_t data_advance1[] = {
        0x8c, 0xf7, 0x0f, 0x0b, // funsel
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x11, // amount 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x40, // offset 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, // size 32 bytes
        // data 0 bytes
    };
    // clang-format on
    cmt_rollup_advance_t advance1 = {.payload = {.length = sizeof(data_advance1), .data = data_advance1}};

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_WITHDRAWAL, NULL, NULL) == -EINVAL);
    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_WITHDRAWAL, &advance1, NULL) == -EINVAL);
    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_WITHDRAWAL, NULL, &parser_input) == -EINVAL);
    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_NONE, &advance1, &parser_input) == -EINVAL);

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_WITHDRAWAL, &advance1, &parser_input) ==
        CMA_PARSER_SUCCESS);
    assert(parser_input.type == CMA_PARSER_INPUT_TYPE_ETHER_WITHDRAWAL);

    // clang-format off
    cma_amount_t amount1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x11,
    }};
    // clang-format on

    assert(memcmp(parser_input.ether_withdrawal.amount.data, amount1.data, CMA_ABI_U256_LENGTH) == 0);
    assert(parser_input.ether_withdrawal.exec_layer_data.length == 0);

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_AUTO, &advance1, &parser_input) == CMA_PARSER_SUCCESS);
    assert(parser_input.type == CMA_PARSER_INPUT_TYPE_ETHER_WITHDRAWAL);
    assert(memcmp(parser_input.ether_withdrawal.amount.data, amount1.data, CMA_ABI_U256_LENGTH) == 0);
    assert(parser_input.ether_withdrawal.exec_layer_data.length == 0);

    // clang-format off
    uint8_t data_advance2[] = {
        0x8c, 0xf7, 0x0f, 0x0b, // funsel
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x11, // amount 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x40, // offset 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x08, // size 32 bytes
        0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef // data 8 bytes
    };
    cmt_rollup_advance_t advance2 = {.payload = { .length = sizeof(data_advance2), .data = data_advance2 }};
    // clang-format on

    // clang-format off
    uint8_t data2[] = {
        0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef
    };
    // clang-format on

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_WITHDRAWAL, &advance2, &parser_input) ==
        CMA_PARSER_SUCCESS);

    assert(memcmp(parser_input.ether_withdrawal.amount.data, amount1.data, CMA_ABI_U256_LENGTH) == 0);
    assert(parser_input.ether_withdrawal.exec_layer_data.length == sizeof(data2));
    assert(memcmp(parser_input.ether_withdrawal.exec_layer_data.data, data2, sizeof(data2)) == 0);

    // clang-format off
    uint8_t malformed_data_advance1[] = {
        0x00, 0x00, // funsel
    };
    uint8_t malformed_data_advance2[] = {
        0x00, 0x00, 0x00, 0x00, // wrong funsel
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x04, // amount 32 bytes
    };
    uint8_t malformed_data_advance3[] = {
        0x8c, 0xf7, 0x0f, 0x0b, // funsel
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x04, // amount 22 bytes
    };
    // clang-format on

    cmt_rollup_advance_t malformed_advance1 = {
        .payload = {.length = sizeof(malformed_data_advance1), .data = malformed_data_advance1}};
    cmt_rollup_advance_t malformed_advance2 = {
        .payload = {.length = sizeof(malformed_data_advance2), .data = malformed_data_advance2}};
    cmt_rollup_advance_t malformed_advance3 = {
        .payload = {.length = sizeof(malformed_data_advance3), .data = malformed_data_advance3}};
    cmt_rollup_advance_t malformed_advance4 = {.payload = {.length = 10, .data = data_advance1}};
    cmt_rollup_advance_t malformed_advance5 = {
        .payload = {.length = sizeof(data_advance2) + 20, .data = data_advance2}};

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_WITHDRAWAL, &malformed_advance1, &parser_input) ==
        -ENOBUFS);

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_WITHDRAWAL, &malformed_advance2, &parser_input) ==
        -EBADMSG);

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_WITHDRAWAL, &malformed_advance3, &parser_input) ==
        -ENOBUFS);

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_WITHDRAWAL, &malformed_advance4, &parser_input) ==
        -ENOBUFS);

    // despite being malformed, the parser doesn't give an error because it wouldn't get
    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_WITHDRAWAL, &malformed_advance5, &parser_input) ==
        CMA_PARSER_SUCCESS);

    printf("%s passed\n", __FUNCTION__);
}

void test_erc20_withdraw(void) {
    cma_parser_input_t parser_input;

    // clang-format off
    uint8_t data_advance1[] = {
        0x4f, 0x94, 0xd3, 0x42, // funsel
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0xff, // token 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x11, // amount 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x60, // offset 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, // size 32 bytes
        // data 0 bytes
    };
    // clang-format on
    cmt_rollup_advance_t advance1 = {.payload = {.length = sizeof(data_advance1), .data = data_advance1}};

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_WITHDRAWAL, NULL, NULL) == -EINVAL);
    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_WITHDRAWAL, &advance1, NULL) == -EINVAL);
    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_WITHDRAWAL, NULL, &parser_input) == -EINVAL);
    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_NONE, &advance1, &parser_input) == -EINVAL);

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_WITHDRAWAL, &advance1, &parser_input) ==
        CMA_PARSER_SUCCESS);
    assert(parser_input.type == CMA_PARSER_INPUT_TYPE_ERC20_WITHDRAWAL);

    // clang-format off
    cma_amount_t amount1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x11,
    }};
    cma_token_address_t token1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
    }};
    // clang-format on

    assert(memcmp(parser_input.erc20_withdrawal.amount.data, amount1.data, CMA_ABI_U256_LENGTH) == 0);
    assert(parser_input.erc20_withdrawal.exec_layer_data.length == 0);

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_AUTO, &advance1, &parser_input) == CMA_PARSER_SUCCESS);
    assert(parser_input.type == CMA_PARSER_INPUT_TYPE_ERC20_WITHDRAWAL);
    assert(memcmp(parser_input.erc20_withdrawal.token.data, token1.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp(parser_input.erc20_withdrawal.amount.data, amount1.data, CMA_ABI_U256_LENGTH) == 0);
    assert(parser_input.erc20_withdrawal.exec_layer_data.length == 0);

    // clang-format off
    uint8_t data_advance2[] = {
        0x4f, 0x94, 0xd3, 0x42, // funsel
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0xff, // token 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x11, // amount 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x60, // offset 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x08, // size 32 bytes
        0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef // data 8 bytes
    };
    cmt_rollup_advance_t advance2 = {.payload = { .length = sizeof(data_advance2), .data = data_advance2 }};
    // clang-format on

    // clang-format off
    uint8_t data2[] = {
        0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef
    };
    // clang-format on

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_WITHDRAWAL, &advance2, &parser_input) ==
        CMA_PARSER_SUCCESS);

    assert(memcmp(parser_input.erc20_withdrawal.token.data, token1.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp(parser_input.erc20_withdrawal.amount.data, amount1.data, CMA_ABI_U256_LENGTH) == 0);
    assert(parser_input.erc20_withdrawal.exec_layer_data.length == sizeof(data2));
    assert(memcmp(parser_input.erc20_withdrawal.exec_layer_data.data, data2, sizeof(data2)) == 0);

    // clang-format off
    uint8_t malformed_data_advance1[] = {
        0x00, 0x00, // funsel
    };
    uint8_t malformed_data_advance2[] = {
        0x00, 0x00, 0x00, 0x00, // wrong funsel
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0xff, // token 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x04, // amount 32 bytes
    };
    uint8_t malformed_data_advance3[] = {
        0x4f, 0x94, 0xd3, 0x42, // funsel
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x04, // token 22 bytes
    };
    uint8_t malformed_data_advance4[] = {
        0x4f, 0x94, 0xd3, 0x42, // funsel
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0xff, // token 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, // amount 30 bytes
    };
    // clang-format on

    cmt_rollup_advance_t malformed_advance1 = {
        .payload = {.length = sizeof(malformed_data_advance1), .data = malformed_data_advance1}};
    cmt_rollup_advance_t malformed_advance2 = {
        .payload = {.length = sizeof(malformed_data_advance2), .data = malformed_data_advance2}};
    cmt_rollup_advance_t malformed_advance3 = {
        .payload = {.length = sizeof(malformed_data_advance3), .data = malformed_data_advance3}};
    cmt_rollup_advance_t malformed_advance4 = {
        .payload = {.length = sizeof(malformed_data_advance3), .data = malformed_data_advance4}};
    cmt_rollup_advance_t malformed_advance5 = {.payload = {.length = 10, .data = data_advance1}};
    cmt_rollup_advance_t malformed_advance6 = {
        .payload = {.length = sizeof(data_advance2) + 20, .data = data_advance2}};

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_WITHDRAWAL, &malformed_advance1, &parser_input) ==
        -ENOBUFS);

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_WITHDRAWAL, &malformed_advance2, &parser_input) ==
        -EBADMSG);

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_WITHDRAWAL, &malformed_advance3, &parser_input) ==
        -ENOBUFS);

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_WITHDRAWAL, &malformed_advance4, &parser_input) ==
        -ENOBUFS);

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_WITHDRAWAL, &malformed_advance5, &parser_input) ==
        -ENOBUFS);

    // despite being malformed, the parser doesn't give an error because it wouldn't get
    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_WITHDRAWAL, &malformed_advance6, &parser_input) ==
        CMA_PARSER_SUCCESS);

    printf("%s passed\n", __FUNCTION__);
}

void test_ether_transfer(void) {
    cma_parser_input_t parser_input;

    // clang-format off
    uint8_t data_advance1[] = {
        0xff, 0x67, 0xc9, 0x03, // funsel
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x02, // receiver 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x11, // amount 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x60, // offset 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, // size 32 bytes
        // data 0 bytes
    };
    // clang-format on
    cmt_rollup_advance_t advance1 = {.payload = {.length = sizeof(data_advance1), .data = data_advance1}};

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_TRANSFER, NULL, NULL) == -EINVAL);
    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_TRANSFER, &advance1, NULL) == -EINVAL);
    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_TRANSFER, NULL, &parser_input) == -EINVAL);
    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_NONE, &advance1, &parser_input) == -EINVAL);

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_TRANSFER, &advance1, &parser_input) ==
        CMA_PARSER_SUCCESS);
    assert(parser_input.type == CMA_PARSER_INPUT_TYPE_ETHER_TRANSFER);

    // clang-format off
    cma_amount_t receiver1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x02,
    }};
    cma_amount_t amount1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x11,
    }};
    // clang-format on

    assert(memcmp(parser_input.ether_transfer.receiver.data, receiver1.data, CMA_ABI_ID_LENGTH) == 0);
    assert(memcmp(parser_input.ether_transfer.amount.data, amount1.data, CMA_ABI_U256_LENGTH) == 0);
    assert(parser_input.ether_transfer.exec_layer_data.length == 0);

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_AUTO, &advance1, &parser_input) == CMA_PARSER_SUCCESS);
    assert(parser_input.type == CMA_PARSER_INPUT_TYPE_ETHER_TRANSFER);
    assert(memcmp(parser_input.ether_transfer.receiver.data, receiver1.data, CMA_ABI_ID_LENGTH) == 0);
    assert(memcmp(parser_input.ether_transfer.amount.data, amount1.data, CMA_ABI_U256_LENGTH) == 0);
    assert(parser_input.ether_transfer.exec_layer_data.length == 0);

    // clang-format off
    uint8_t data_advance2[] = {
        0xff, 0x67, 0xc9, 0x03, // funsel
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x02, // receiver 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x11, // amount 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x60, // offset 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x08, // size 32 bytes
        0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef // data 8 bytes
    };
    cmt_rollup_advance_t advance2 = {.payload = { .length = sizeof(data_advance2), .data = data_advance2 }};
    // clang-format on

    // clang-format off
    uint8_t data2[] = {
        0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef
    };
    // clang-format on

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_TRANSFER, &advance2, &parser_input) ==
        CMA_PARSER_SUCCESS);

    assert(memcmp(parser_input.ether_transfer.amount.data, amount1.data, CMA_ABI_U256_LENGTH) == 0);
    assert(parser_input.ether_transfer.exec_layer_data.length == sizeof(data2));
    assert(memcmp(parser_input.ether_transfer.exec_layer_data.data, data2, sizeof(data2)) == 0);

    // clang-format off
    uint8_t malformed_data_advance1[] = {
        0x00, 0x00, // funsel
    };
    uint8_t malformed_data_advance2[] = {
        0x00, 0x00, 0x00, 0x00, // wrong funsel
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x04, // receiver 32 bytes
    };
    uint8_t malformed_data_advance3[] = {
        0xff, 0x67, 0xc9, 0x03, // funsel
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x04, // receiver 22 bytes
    };
    // clang-format on

    cmt_rollup_advance_t malformed_advance1 = {
        .payload = {.length = sizeof(malformed_data_advance1), .data = malformed_data_advance1}};
    cmt_rollup_advance_t malformed_advance2 = {
        .payload = {.length = sizeof(malformed_data_advance2), .data = malformed_data_advance2}};
    cmt_rollup_advance_t malformed_advance3 = {
        .payload = {.length = sizeof(malformed_data_advance3), .data = malformed_data_advance3}};
    cmt_rollup_advance_t malformed_advance4 = {.payload = {.length = 10, .data = data_advance1}};
    cmt_rollup_advance_t malformed_advance5 = {
        .payload = {.length = sizeof(data_advance2) + 20, .data = data_advance2}};

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_TRANSFER, &malformed_advance1, &parser_input) ==
        -ENOBUFS);

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_TRANSFER, &malformed_advance2, &parser_input) ==
        -EBADMSG);

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_TRANSFER, &malformed_advance3, &parser_input) ==
        -ENOBUFS);

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_TRANSFER, &malformed_advance4, &parser_input) ==
        -ENOBUFS);

    // despite being malformed, the parser doesn't give an error because it wouldn't get
    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_TRANSFER, &malformed_advance5, &parser_input) ==
        CMA_PARSER_SUCCESS);

    printf("%s passed\n", __FUNCTION__);
}

void test_erc20_transfer(void) {
    cma_parser_input_t parser_input;

    // clang-format off
    uint8_t data_advance1[] = {
        0x03, 0xd6, 0x1d, 0xcd, // funsel
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0xff, // token 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x02, // receiver 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x11, // amount 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x80, // offset 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, // size 32 bytes
        // data 0 bytes
    };
    // clang-format on
    cmt_rollup_advance_t advance1 = {.payload = {.length = sizeof(data_advance1), .data = data_advance1}};

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_TRANSFER, NULL, NULL) == -EINVAL);
    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_TRANSFER, &advance1, NULL) == -EINVAL);
    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_TRANSFER, NULL, &parser_input) == -EINVAL);
    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_NONE, &advance1, &parser_input) == -EINVAL);

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_TRANSFER, &advance1, &parser_input) ==
        CMA_PARSER_SUCCESS);
    assert(parser_input.type == CMA_PARSER_INPUT_TYPE_ERC20_TRANSFER);

    // clang-format off
    cma_amount_t receiver1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x02,
    }};
    cma_token_address_t token1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
    }};
    cma_amount_t amount1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x11,
    }};
    // clang-format on

    assert(memcmp(parser_input.erc20_transfer.receiver.data, receiver1.data, CMA_ABI_ID_LENGTH) == 0);
    assert(memcmp(parser_input.erc20_transfer.token.data, token1.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp(parser_input.erc20_transfer.amount.data, amount1.data, CMA_ABI_U256_LENGTH) == 0);
    assert(parser_input.erc20_transfer.exec_layer_data.length == 0);

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_AUTO, &advance1, &parser_input) == CMA_PARSER_SUCCESS);
    assert(parser_input.type == CMA_PARSER_INPUT_TYPE_ERC20_TRANSFER);
    assert(memcmp(parser_input.erc20_transfer.receiver.data, receiver1.data, CMA_ABI_ID_LENGTH) == 0);
    assert(memcmp(parser_input.erc20_transfer.token.data, token1.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp(parser_input.erc20_transfer.amount.data, amount1.data, CMA_ABI_U256_LENGTH) == 0);
    assert(parser_input.erc20_transfer.exec_layer_data.length == 0);

    // clang-format off
    uint8_t data_advance2[] = {
        0x03, 0xd6, 0x1d, 0xcd, // funsel
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0xff, // token 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x02, // receiver 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x11, // amount 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x80, // offset 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x08, // size 32 bytes
        0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef // data 8 bytes
    };
    cmt_rollup_advance_t advance2 = {.payload = { .length = sizeof(data_advance2), .data = data_advance2 }};
    // clang-format on

    // clang-format off
    uint8_t data2[] = {
        0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef
    };
    // clang-format on

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_TRANSFER, &advance2, &parser_input) ==
        CMA_PARSER_SUCCESS);

    assert(memcmp(parser_input.erc20_transfer.token.data, token1.data, CMA_ABI_ADDRESS_LENGTH) == 0);
    assert(memcmp(parser_input.erc20_transfer.amount.data, amount1.data, CMA_ABI_U256_LENGTH) == 0);
    assert(parser_input.erc20_transfer.exec_layer_data.length == sizeof(data2));
    assert(memcmp(parser_input.erc20_transfer.exec_layer_data.data, data2, sizeof(data2)) == 0);

    // clang-format off
    uint8_t malformed_data_advance1[] = {
        0x00, 0x00, // funsel
    };
    uint8_t malformed_data_advance2[] = {
        0x00, 0x00, 0x00, 0x00, // wrong funsel
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x04, // receiver 32 bytes
    };
    uint8_t malformed_data_advance3[] = {
        0x03, 0xd6, 0x1d, 0xcd, // funsel
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x04, // token 22 bytes
    };
    uint8_t malformed_data_advance4[] = {
        0x03, 0xd6, 0x1d, 0xcd, // funsel
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0xff, // token 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x04, // receiver 22 bytes
    };
    // clang-format on

    cmt_rollup_advance_t malformed_advance1 = {
        .payload = {.length = sizeof(malformed_data_advance1), .data = malformed_data_advance1}};
    cmt_rollup_advance_t malformed_advance2 = {
        .payload = {.length = sizeof(malformed_data_advance2), .data = malformed_data_advance2}};
    cmt_rollup_advance_t malformed_advance3 = {
        .payload = {.length = sizeof(malformed_data_advance3), .data = malformed_data_advance3}};
    cmt_rollup_advance_t malformed_advance4 = {
        .payload = {.length = sizeof(malformed_data_advance3), .data = malformed_data_advance4}};
    cmt_rollup_advance_t malformed_advance5 = {.payload = {.length = 10, .data = data_advance1}};
    cmt_rollup_advance_t malformed_advance6 = {
        .payload = {.length = sizeof(data_advance2) + 20, .data = data_advance2}};

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_TRANSFER, &malformed_advance1, &parser_input) ==
        -ENOBUFS);

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_TRANSFER, &malformed_advance2, &parser_input) ==
        -EBADMSG);

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_TRANSFER, &malformed_advance3, &parser_input) ==
        -ENOBUFS);

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_TRANSFER, &malformed_advance4, &parser_input) ==
        -ENOBUFS);

    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_TRANSFER, &malformed_advance5, &parser_input) ==
        -ENOBUFS);

    // despite being malformed, the parser doesn't give an error because it wouldn't get
    assert(cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_TRANSFER, &malformed_advance6, &parser_input) ==
        CMA_PARSER_SUCCESS);

    printf("%s passed\n", __FUNCTION__);
}

void test_ether_voucher(void) {
    // clang-format off
    cma_parser_voucher_data_t voucher_req = { .receiver = { .data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, // address 20 bytes
    }}, .ether = { .amount = { .data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x11, // amount 32 bytes
    }}}};
    uint8_t voucher_payload_buffer[0];
    cma_voucher_t voucher = { .payload = {
        .length = sizeof(voucher_payload_buffer),
        .data = voucher_payload_buffer
    }};
    // clang-format on

    assert(cma_parser_encode_voucher(CMA_PARSER_VOUCHER_TYPE_ETHER, NULL, NULL, NULL) == -EINVAL);
    assert(cma_parser_encode_voucher(CMA_PARSER_VOUCHER_TYPE_ETHER, NULL, &voucher_req, NULL) == -EINVAL);
    assert(cma_parser_encode_voucher(CMA_PARSER_VOUCHER_TYPE_ETHER, NULL, NULL, &voucher) == -EINVAL);

    assert(cma_parser_encode_voucher(CMA_PARSER_VOUCHER_TYPE_NONE, NULL, &voucher_req, &voucher) == -EINVAL);
    assert(
        cma_parser_encode_voucher(CMA_PARSER_VOUCHER_TYPE_ETHER, NULL, &voucher_req, &voucher) == CMA_PARSER_SUCCESS);

    // clang-format off
    cma_abi_address_t receiver1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    }};
    cma_amount_t amount1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x11,
    }};
    // clang-format on

    assert(voucher.payload.length == 0);
    assert(voucher.payload.data == NULL);

    assert(memcmp(voucher.address.data, receiver1.data, sizeof(receiver1)) == 0);
    assert(memcmp(voucher.value.data, amount1.data, sizeof(amount1)) == 0);

    // clang-format off
    cma_abi_address_t address2 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23,
    }};
    // clang-format on

    assert(cma_parser_encode_voucher(CMA_PARSER_VOUCHER_TYPE_ETHER, &address2, &voucher_req, &voucher) ==
        CMA_PARSER_SUCCESS);

    printf("%s passed\n", __FUNCTION__);
}

void test_erc20_voucher(void) {
    // clang-format off
    cma_parser_voucher_data_t voucher_req = { .receiver = { .data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, // address 20 bytes
    }}, .erc20 = { .token = { .data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, // token 20 bytes
    }}, .amount = { .data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x11, // amount 32 bytes
    }}}};
    uint8_t voucher_payload_buffer[CMA_PARSER_ERC20_VOUCHER_PAYLOAD_SIZE];
    cma_voucher_t voucher = { .payload = {
        .length = sizeof(voucher_payload_buffer),
        .data = voucher_payload_buffer
    }};
    // clang-format on

    assert(cma_parser_encode_voucher(CMA_PARSER_VOUCHER_TYPE_ERC20, NULL, NULL, NULL) == -EINVAL);
    assert(cma_parser_encode_voucher(CMA_PARSER_VOUCHER_TYPE_ERC20, NULL, &voucher_req, NULL) == -EINVAL);
    assert(cma_parser_encode_voucher(CMA_PARSER_VOUCHER_TYPE_ERC20, NULL, NULL, &voucher) == -EINVAL);

    assert(cma_parser_encode_voucher(CMA_PARSER_VOUCHER_TYPE_NONE, NULL, &voucher_req, &voucher) == -EINVAL);
    assert(
        cma_parser_encode_voucher(CMA_PARSER_VOUCHER_TYPE_ERC20, NULL, &voucher_req, &voucher) == CMA_PARSER_SUCCESS);

    // clang-format off
    cma_abi_address_t receiver1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    }};
    cma_abi_address_t token1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
    }};
    cma_amount_t amount1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00,
    }};
    uint8_t voucher_payload[] = {
        0xa9, 0x05, 0x9c, 0xbb, // funsel
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0xff, // token 32 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x11, // amount 32 bytes
    };
    // clang-format on

    assert(voucher.payload.length == CMA_PARSER_ERC20_VOUCHER_PAYLOAD_SIZE);
    assert(memcmp(voucher.payload.data, voucher_payload, sizeof(voucher.payload.length)) == 0);
    assert(memcmp(voucher.address.data, token1.data, sizeof(receiver1)) == 0);
    assert(memcmp(voucher.value.data, amount1.data, sizeof(amount1)) == 0);

    // clang-format off
    cma_abi_address_t address2 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23,
    }};
    // clang-format on

    assert(cma_parser_encode_voucher(CMA_PARSER_VOUCHER_TYPE_ERC20, &address2, &voucher_req, &voucher) ==
        CMA_PARSER_SUCCESS);

    printf("%s passed\n", __FUNCTION__);
}

void test_get_balance(void) {
    char *payload1 = "{\"method\":\"ledger_getBalance\",\"params\":[\"0x0000000000000000000000000000000000000001\"]}";
    cmt_rollup_inspect_t inspect1 = {.payload = {.length = strlen(payload1), .data = payload1}};

    cma_parser_input_t parser_input;

    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_BALANCE, NULL, NULL) == -EINVAL);
    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_BALANCE, NULL, &parser_input) == -EINVAL);
    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_BALANCE, &inspect1, NULL) == -EINVAL);

    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_BALANCE, &inspect1, &parser_input) == CMA_PARSER_SUCCESS);

    // clang-format off
    cma_account_id_t account1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01,
    }};
    // clang-format on

    assert(parser_input.type == CMA_PARSER_INPUT_TYPE_BALANCE_ACCOUNT);
    assert(memcmp(parser_input.balance.account.data, account1.data, sizeof(account1)) == 0);

    char *payload2 = "{\"method\":\"ledger_getBalance\",\"params\":["
                     "\"0x0000000000000000000000000000000000000000000000000000000000000001\"]}";
    cmt_rollup_inspect_t inspect2 = {.payload = {.length = strlen(payload2), .data = payload2}};

    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_BALANCE, &inspect2, &parser_input) == CMA_PARSER_SUCCESS);
    assert(parser_input.type == CMA_PARSER_INPUT_TYPE_BALANCE_ACCOUNT);
    assert(memcmp(parser_input.balance.account.data, account1.data, sizeof(account1)) == 0);

    char *payload3 = "{\"method\":\"ledger_getBalance\",\"params\":[\"0x0000000000000000000000000000000000000001\", "
                     "\"0x00000000000000000000000000000000000000ff\"]}";
    cmt_rollup_inspect_t inspect3 = {.payload = {.length = strlen(payload3), .data = payload3}};

    // clang-format off
    cma_abi_address_t token1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
    }};
    // clang-format on

    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_BALANCE, &inspect3, &parser_input) == CMA_PARSER_SUCCESS);
    assert(parser_input.type == CMA_PARSER_INPUT_TYPE_BALANCE_ACCOUNT_TOKEN_ADDRESS);
    assert(memcmp(parser_input.balance.account.data, account1.data, sizeof(account1)) == 0);
    assert(memcmp(parser_input.balance.token.data, token1.data, sizeof(token1)) == 0);

    char *payload4 = "{\"method\":\"ledger_getBalance\",\"params\":[\"0x0000000000000000000000000000000000000001\", "
                     "\"0x00000000000000000000000000000000000000ff\","
                     "\"0x0000000000000000000000000000000000000000000000000000000000000011\"]}";
    cmt_rollup_inspect_t inspect4 = {.payload = {.length = strlen(payload4), .data = payload4}};

    // clang-format off
    cma_token_id_t token_id1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x11,
    }};
    // clang-format on

    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_BALANCE, &inspect4, &parser_input) == CMA_PARSER_SUCCESS);
    assert(parser_input.type == CMA_PARSER_INPUT_TYPE_BALANCE_ACCOUNT_TOKEN_ADDRESS_ID);
    assert(memcmp(parser_input.balance.account.data, account1.data, sizeof(account1)) == 0);
    assert(memcmp(parser_input.balance.token.data, token1.data, sizeof(token1)) == 0);
    assert(memcmp(parser_input.balance.token_id.data, token_id1.data, sizeof(token_id1)) == 0);

    char *payload4b = "{\"method\":\"ledger_getBalance\",\"params\":[\"0x0000000000000000000000000000000000000001\", "
                      "\"0x00000000000000000000000000000000000000ff\",\"0x11\"]}";
    cmt_rollup_inspect_t inspect4b = {.payload = {.length = strlen(payload4b), .data = payload4b}};
    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_BALANCE, &inspect4b, &parser_input) == CMA_PARSER_SUCCESS);
    assert(memcmp(parser_input.balance.token_id.data, token_id1.data, sizeof(token_id1)) == 0);

    char *payload4c = "{\"method\":\"ledger_getBalance\",\"params\":[\"0x0000000000000000000000000000000000000001\", "
                      "\"0x00000000000000000000000000000000000000ff\",\"0x11\",\"abc\"]}";
    cmt_rollup_inspect_t inspect4c = {.payload = {.length = strlen(payload4c), .data = payload4c}};
    char *eld4c = "abc";
    // clang-format off
    uint8_t exec_layer_data_buffer[200];
    cma_parser_input_t parser_input4c = { .balance = { .exec_layer_data = {
        .length = sizeof(exec_layer_data_buffer),
        .data = exec_layer_data_buffer
    }}};
    // clang-format on

    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_BALANCE, &inspect4c, &parser_input4c) == CMA_PARSER_SUCCESS);
    assert(memcmp(parser_input4c.balance.token_id.data, token_id1.data, sizeof(token_id1)) == 0);
    assert(
        memcmp(parser_input4c.balance.exec_layer_data.data, eld4c, parser_input4c.balance.exec_layer_data.length) == 0);

    char *m_payload5 = "abc";
    cmt_rollup_inspect_t m_inspect5 = {.payload = {.length = strlen(m_payload5), .data = m_payload5}};
    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_BALANCE, &m_inspect5, &parser_input) == -EINVAL);

    char *m_payload6 = "{\"params\":[\"0x0000000000000000000000000000000000000001\", "
                       "\"0x00000000000000000000000000000000000000ff\","
                       "\"0x0000000000000000000000000000000000000000000000000000000000000011\"]}";
    cmt_rollup_inspect_t m_inspect6 = {.payload = {.length = strlen(m_payload6), .data = m_payload6}};
    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_BALANCE, &m_inspect6, &parser_input) == -EINVAL);

    char *m_payload7 = "{\"method\":\"ledger_getBalance\"}";
    cmt_rollup_inspect_t m_inspect7 = {.payload = {.length = strlen(m_payload7), .data = m_payload7}};
    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_BALANCE, &m_inspect7, &parser_input) == -EINVAL);

    char *m_payload8 = "{\"method\":\"ledger_getBal\",\"params\":[\"0x0000000000000000000000000000000000000001\", "
                       "\"0x00000000000000000000000000000000000000ff\","
                       "\"0x0000000000000000000000000000000000000000000000000000000000000011\"]}";
    cmt_rollup_inspect_t m_inspect8 = {.payload = {.length = strlen(m_payload8), .data = m_payload8}};
    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_BALANCE, &m_inspect8, &parser_input) == -EINVAL);

    char *m_payload9 = "{\"method\":\"ledger_getBalance\",\"params\":[\"0x00000000000000000000000000000000000001\", "
                       "\"0x00000000000000000000000000000000000000ff\","
                       "\"0x0000000000000000000000000000000000000000000000000000000000000011\"]}";
    cmt_rollup_inspect_t m_inspect9 = {.payload = {.length = strlen(m_payload9), .data = m_payload9}};
    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_BALANCE, &m_inspect9, &parser_input) == -EINVAL);

    char *m_payload10 = "{\"method\":\"ledger_getBalance\",\"params\":[\"0x0000000000000000000000000000000000000001\", "
                        "\"\", \"0x0000000000000000000000000000000000000000000000000000000000000011\"]}";
    cmt_rollup_inspect_t m_inspect10 = {.payload = {.length = strlen(m_payload10), .data = m_payload10}};
    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_BALANCE, &m_inspect10, &parser_input) == -EINVAL);

    char *m_payload11 = "{\"method\":\"ledger_getBalance\",\"params\":[\"0x0000000000000000000000000000000000000001\", "
                        "\"0x00000000000000000000000000000000000000ff\","
                        "\"0x000000000000000000000000000000000000000000000000000000000000000011\"]}";
    cmt_rollup_inspect_t m_inspect11 = {.payload = {.length = strlen(m_payload11), .data = m_payload11}};
    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_BALANCE, &m_inspect11, &parser_input) == -EINVAL);

    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_AUTO, &inspect2, &parser_input) == CMA_PARSER_SUCCESS);
    assert(parser_input.type == CMA_PARSER_INPUT_TYPE_BALANCE_ACCOUNT);
    assert(memcmp(parser_input.balance.account.data, account1.data, sizeof(account1)) == 0);

    printf("%s passed\n", __FUNCTION__);
}

void test_get_total_supply(void) {
    char *payload1 = "{\"method\":\"ledger_getTotalSupply\"}";
    cmt_rollup_inspect_t inspect1 = {.payload = {.length = strlen(payload1), .data = payload1}};

    cma_parser_input_t parser_input;

    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_SUPPLY, NULL, NULL) == -EINVAL);
    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_SUPPLY, NULL, &parser_input) == -EINVAL);
    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_SUPPLY, &inspect1, NULL) == -EINVAL);

    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_SUPPLY, &inspect1, &parser_input) == CMA_PARSER_SUCCESS);

    assert(parser_input.type == CMA_PARSER_INPUT_TYPE_SUPPLY);

    char *payload2 = "{\"method\":\"ledger_getTotalSupply\",\"params\":[]}";
    cmt_rollup_inspect_t inspect2 = {.payload = {.length = strlen(payload2), .data = payload2}};

    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_SUPPLY, &inspect2, &parser_input) == CMA_PARSER_SUCCESS);
    assert(parser_input.type == CMA_PARSER_INPUT_TYPE_SUPPLY);

    char *payload3 =
        "{\"method\":\"ledger_getTotalSupply\",\"params\":[\"0x00000000000000000000000000000000000000ff\"]}";
    cmt_rollup_inspect_t inspect3 = {.payload = {.length = strlen(payload3), .data = payload3}};

    // clang-format off
    cma_abi_address_t token1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
    }};
    // clang-format on

    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_SUPPLY, &inspect3, &parser_input) == CMA_PARSER_SUCCESS);
    assert(parser_input.type == CMA_PARSER_INPUT_TYPE_SUPPLY_TOKEN_ADDRESS);
    assert(memcmp(parser_input.supply.token.data, token1.data, sizeof(token1)) == 0);

    char *payload4 = "{\"method\":\"ledger_getTotalSupply\",\"params\":[\"0x00000000000000000000000000000000000000ff\","
                     "\"0x0000000000000000000000000000000000000000000000000000000000000011\"]}";
    cmt_rollup_inspect_t inspect4 = {.payload = {.length = strlen(payload4), .data = payload4}};

    // clang-format off
    cma_token_id_t token_id1 = {.data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x11,
    }};
    // clang-format on

    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_SUPPLY, &inspect4, &parser_input) == CMA_PARSER_SUCCESS);
    assert(parser_input.type == CMA_PARSER_INPUT_TYPE_SUPPLY_TOKEN_ADDRESS_ID);
    assert(memcmp(parser_input.supply.token.data, token1.data, sizeof(token1)) == 0);
    assert(memcmp(parser_input.supply.token_id.data, token_id1.data, sizeof(token_id1)) == 0);

    char *payload4b =
        "{\"method\":\"ledger_getTotalSupply\",\"params\":[\"0x00000000000000000000000000000000000000ff\",\"0x11\"]}";
    cmt_rollup_inspect_t inspect4b = {.payload = {.length = strlen(payload4b), .data = payload4b}};
    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_SUPPLY, &inspect4b, &parser_input) == CMA_PARSER_SUCCESS);
    assert(memcmp(parser_input.supply.token_id.data, token_id1.data, sizeof(token_id1)) == 0);

    char *payload4c = "{\"method\":\"ledger_getTotalSupply\",\"params\":["
                      "\"0x00000000000000000000000000000000000000ff\",\"0x11\",\"abc\"]}";
    cmt_rollup_inspect_t inspect4c = {.payload = {.length = strlen(payload4c), .data = payload4c}};
    char *eld4c = "abc";
    // clang-format off
    uint8_t exec_layer_data_buffer[200];
    cma_parser_input_t parser_input4c = { .supply = { .exec_layer_data = {
        .length = sizeof(exec_layer_data_buffer),
        .data = exec_layer_data_buffer
    }}};
    // clang-format on

    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_SUPPLY, &inspect4c, &parser_input4c) == CMA_PARSER_SUCCESS);
    assert(memcmp(parser_input4c.supply.token_id.data, token_id1.data, sizeof(token_id1)) == 0);
    assert(
        memcmp(parser_input4c.supply.exec_layer_data.data, eld4c, parser_input4c.supply.exec_layer_data.length) == 0);

    char *m_payload5 = "abc";
    cmt_rollup_inspect_t m_inspect5 = {.payload = {.length = strlen(m_payload5), .data = m_payload5}};
    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_SUPPLY, &m_inspect5, &parser_input) == -EINVAL);

    char *m_payload6 = "{\"params\":[\"0x00000000000000000000000000000000000000ff\","
                       "\"0x0000000000000000000000000000000000000000000000000000000000000011\"]}";
    cmt_rollup_inspect_t m_inspect6 = {.payload = {.length = strlen(m_payload6), .data = m_payload6}};
    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_SUPPLY, &m_inspect6, &parser_input) == -EINVAL);

    char *m_payload7 =
        "{\"method\":\"ledger_getTotalSupply\",\"params\":[\"0x00000000000000000000000000000000000000ff\","
        "\"0x000000000000000000000000000000000000000000000000000000000000000011\"]}";
    cmt_rollup_inspect_t m_inspect7 = {.payload = {.length = strlen(m_payload7), .data = m_payload7}};
    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_SUPPLY, &m_inspect7, &parser_input) == -EINVAL);

    char *m_payload8 = "{\"method\":\"ledger_getTot\",\"params\":[\"0x00000000000000000000000000000000000000ff\","
                       "\"0x0000000000000000000000000000000000000000000000000000000000000011\"]}";
    cmt_rollup_inspect_t m_inspect8 = {.payload = {.length = strlen(m_payload8), .data = m_payload8}};
    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_SUPPLY, &m_inspect8, &parser_input) == -EINVAL);

    char *m_payload9 = "{\"method\":\"ledger_getTotalSupply\",\"params\":[\"\", "
                       "\"0x0000000000000000000000000000000000000000000000000000000000000011\"]}";
    cmt_rollup_inspect_t m_inspect9 = {.payload = {.length = strlen(m_payload9), .data = m_payload9}};
    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_SUPPLY, &m_inspect9, &parser_input) == -EINVAL);

    assert(cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_AUTO, &inspect2, &parser_input) == CMA_PARSER_SUCCESS);
    assert(parser_input.type == CMA_PARSER_INPUT_TYPE_SUPPLY);

    printf("%s passed\n", __FUNCTION__);
}

int main(void) {
    test_ether_deposit();
    test_erc20_deposit();
    test_ether_withdraw();
    test_erc20_withdraw();
    test_ether_transfer();
    test_erc20_transfer();
    test_ether_voucher();
    test_erc20_voucher();
    test_get_balance();
    test_get_total_supply();
    printf("All parser tests passed!\n");
    return 0;
}

// printf("sender: ");
// for (size_t i = 0; i < sizeof(parser_input.ether_deposit.sender.data); i++) {
//     printf("%02x", parser_input.ether_deposit.sender.data[i]);
// }
// printf(" = ");
// for (size_t i = 0; i < sizeof(address1.data); i++) {
//     printf("%02x", address1.data[i]);
// }
// printf(" (expected)\n");

// printf("res %d (%s)\n",
//     cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_DEPOSIT, &malformed_advance1, &parser_input),
//     cma_parser_get_last_error_message());
