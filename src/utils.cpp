#include <cstdint>
#include <cstdio>

#include "utils.h"

bool amount_checked_add(cma_amount_t &res, const cma_amount_t &a, const cma_amount_t &b) {
    uint16_t carry = 0;
    for (size_t i = 0; i < sizeof(res.data); ++i) {
        const size_t j = sizeof(res.data) - i - 1;
        const uint16_t aj = a.data[j];
        const uint16_t bj = b.data[j];
        const uint16_t tmp = carry + aj + bj;
        res.data[j] = static_cast<uint8_t>(tmp);
        carry = tmp >> 8U;
    }
    return carry == 0;
}

bool amount_checked_sub(cma_amount_t &res, const cma_amount_t &a, const cma_amount_t &b) {
    uint16_t borrow = 0;
    const uint16_t fix_borrow = 1 << 8U;
    for (size_t i = 0; i < sizeof(res.data); ++i) {
        const size_t j = sizeof(res.data) - i - 1;
        const uint16_t aj = a.data[j];
        const uint16_t bj = b.data[j];
        uint16_t tmp = 0;
        if (borrow + bj > aj) {
            tmp = aj + fix_borrow - borrow - bj;
            borrow = 1;
        } else {
            tmp = aj - borrow - bj;
            borrow = 0;
        }
        res.data[j] = static_cast<uint8_t>(tmp);
    }
    return borrow == 0;
}
