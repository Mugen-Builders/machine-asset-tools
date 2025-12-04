#include <cstddef>
#include <cstdint>
#include <cstdio>

extern "C" {
#include "libcma/types.h"
}

#include "utils.h"

constexpr uint16_t carry_shift = 8U;

auto amount_checked_add(cma_amount_t &res, const cma_amount_t &amount_a, const cma_amount_t &amount_b) -> bool {
    uint16_t carry = 0;
    for (size_t i = 0; i < sizeof(res.data); ++i) {
        const size_t j_ind = sizeof(res.data) - i - 1;
        const uint16_t a_j = amount_a.data[j_ind];
        const uint16_t b_j = amount_b.data[j_ind];
        const uint16_t tmp = carry + a_j + b_j;
        res.data[j_ind] = static_cast<uint8_t>(tmp);
        carry = tmp >> carry_shift;
    }
    return carry == 0;
}

auto amount_checked_sub(cma_amount_t &res, const cma_amount_t &amount_a, const cma_amount_t &amount_b) -> bool {
    uint16_t borrow = 0;
    const uint16_t fix_borrow = 1 << carry_shift;
    for (size_t i = 0; i < sizeof(res.data); ++i) {
        const size_t j_ind = sizeof(res.data) - i - 1;
        const uint16_t a_j = amount_a.data[j_ind];
        const uint16_t b_j = amount_b.data[j_ind];
        uint16_t tmp = 0;
        if (borrow + b_j > a_j) {
            tmp = a_j + fix_borrow - borrow - b_j;
            borrow = 1;
        } else {
            tmp = a_j - borrow - b_j;
            borrow = 0;
        }
        res.data[j_ind] = static_cast<uint8_t>(tmp);
    }
    return borrow == 0;
}
