#ifndef CMA_UTILS_H
#define CMA_UTILS_H

#include <exception>
#include <string> // for string class

extern "C" {
#include "libcma/types.h"
}

auto amount_checked_add(cma_amount_t &res, const cma_amount_t &amount_a, const cma_amount_t &amount_b) -> bool;
auto amount_checked_sub(cma_amount_t &res, const cma_amount_t &amount_a, const cma_amount_t &amount_b) -> bool;

// Custom exception class
class CmaException : public std::exception {
private:
    std::string message;
    int errorCode;

public:
    // Constructor to initialize message and error code
    CmaException(std::string msg, int code) : message(std::move(msg)), errorCode(code) {}

    // Override the what() method to return the error message
    [[nodiscard]]
    auto what() const noexcept -> const char * override {
        return message.c_str();
    }

    // Method to get the custom error code
    [[nodiscard]]
    auto code() const noexcept -> int {
        return errorCode;
    }
};

#endif // CMA_UTILS_H
