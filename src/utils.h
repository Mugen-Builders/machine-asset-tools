#ifndef CMA_UTILS_H
#define CMA_UTILS_H

extern "C" {
#include "libcma/types.h"
}

bool amount_checked_add(cma_amount_t &res, const cma_amount_t &a, const cma_amount_t &b);
bool amount_checked_sub(cma_amount_t &res, const cma_amount_t &a, const cma_amount_t &b);

#endif // CMA_UTILS_H
