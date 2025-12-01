#ifndef CMA_LEDGER_IMPL_H
#define CMA_LEDGER_IMPL_H

#include <string> // for string class
#include <unordered_map>

extern "C" {
#include "libcma/ledger.h"
}

enum {
    CMA_ABI_ADDRESS_LENGTH = CMT_ABI_ADDRESS_LENGTH,
    CMA_ABI_U256_LENGTH = CMT_ABI_U256_LENGTH,
    CMA_ABI_ID_LENGTH = CMT_ABI_U256_LENGTH,

    CMA_LEDGER_ASSET_TYPE_SIZE = 1,
    CMA_LEDGER_ASSET_ARRAY_KEY_TYPE_IND = 0,
    CMA_LEDGER_ASSET_ARRAY_KEY_ADDRESS_IND = CMA_LEDGER_ASSET_ARRAY_KEY_TYPE_IND + CMA_LEDGER_ASSET_TYPE_SIZE,
    CMA_LEDGER_ASSET_ARRAY_KEY_ID_IND = CMA_LEDGER_ASSET_ARRAY_KEY_ADDRESS_IND + CMA_ABI_ADDRESS_LENGTH,

};

enum : uint64_t {
    CMA_LEDGER_MAGIC = 0x6de6c7b338afbad6,
};

// Custom exception class
class LedgerException : public std::exception {
private:
    std::string message;
    int errorCode;

public:
    // Constructor to initialize message and error code
    LedgerException(const std::string &msg, int code) : message(msg), errorCode(code) {}

    // Override the what() method to return the error message
    const char *what() const noexcept override {
        return message.c_str();
    }

    // Method to get the custom error code
    int code() const noexcept {
        return errorCode;
    }
};

using cma_ledger_asset_struct_t = struct cma_ledger_asset_struct {
    cma_ledger_asset_type_t type;
    cma_token_address_t token_address;
    cma_token_id_t token_id;
    cma_amount_t supply;

    // // Overload operator== for comparison if needed for other contexts
    // //   to use it have to consider don't care cases
    // bool operator==(const cma_ledger_asset_struct &other) const {
    //     return type == other.type &&
    //         memcmp(&token_address.data, &other.token_address.data, sizeof(token_address.data)) == 0 &&
    //         memcmp(&token_id.data, &other.token_id.data, sizeof(token_id.data)) == 0 &&
    //         memcmp(&supply.data, &other.supply.data, sizeof(supply.data)) == 0;
    // }
};

// bool operator==(const bytes32_t &a, const bytes32_t &b) {
//     return std::equal(std::begin(a.data), std::end(a.data), std::begin(b.data));
// }

// // Overload operator== for comparison if needed for other contexts
// //   to use it have to consider don't care cases
// bool operator==(const cma_ledger_account_struct_t &a, const cma_ledger_account_struct_t &b) const {
//     return a.type == b.type && memcmp(&a.account_id.data, &b.account_id.data, sizeof(a.account_id.data)) == 0;
// }

struct hash_pair {
    template <class T1, class T2>
    size_t operator()(const std::pair<T1, T2> &p) const {
        size_t hash1 = std::hash<T1>{}(p.first);
        size_t hash2 = std::hash<T2>{}(p.second);
        return hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
    }
};

using cma_ledger_asset_key_bytes_t = uint8_t[CMA_LEDGER_ASSET_TYPE_SIZE + CMA_ABI_ADDRESS_LENGTH + CMA_ABI_ID_LENGTH];
using cma_ledger_asset_key_t = std::string;
using cma_ledger_account_key_t = std::string;

using cma_map_key_t = std::pair<cma_ledger_account_id_t, cma_ledger_asset_id_t>;

using lassid_to_asset_t = std::unordered_map<cma_ledger_asset_id_t, cma_ledger_asset_struct_t>;
using asset_to_lassid_t = std::unordered_map<cma_ledger_asset_key_t, cma_ledger_asset_id_t>;
using laccid_to_account_t = std::unordered_map<cma_ledger_account_id_t, cma_ledger_account_t>;
using account_to_laccid_t = std::unordered_map<cma_ledger_account_key_t, cma_ledger_account_id_t>;
using account_asset_map_t = std::unordered_map<cma_map_key_t, cma_amount_t, hash_pair>;

class cma_ledger {
private:
    uint64_t magic;
    lassid_to_asset_t lassid_to_asset;
    asset_to_lassid_t asset_to_lassid;
    laccid_to_account_t laccid_to_account;
    account_to_laccid_t account_to_laccid;
    account_asset_map_t account_asset_balance;

public:
    cma_ledger() :
        magic(CMA_LEDGER_MAGIC),
        lassid_to_asset(),
        asset_to_lassid(),
        laccid_to_account(),
        account_to_laccid(),
        account_asset_balance() {}
    ~cma_ledger();
    bool is_initialized() const;
    void clear();

    size_t get_asset_count();
    void retrieve_create_asset(cma_ledger_asset_id_t *asset_id, cma_token_address_t *token_address,
        cma_token_id_t *token_id, cma_ledger_asset_type_t asset_type, cma_ledger_retrieve_operation_t op);
    bool find_asset(cma_ledger_asset_id_t asset_id, cma_token_address_t *token_address, cma_token_id_t *token_id,
        cma_amount_t *supply);

    size_t get_account_count();
    bool find_account(cma_ledger_account_id_t account_id, cma_ledger_account_t *account);
    void retrieve_create_account(cma_ledger_account_id_t *account_id, cma_ledger_account_t *account,
        const void *addr_accid, cma_ledger_account_type_t account_type, cma_ledger_retrieve_operation_t op);

    void set_asset_supply(cma_ledger_asset_id_t asset_id, cma_amount_t &supply);
    void get_account_asset_balance(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t to_account_id,
        cma_amount_t &balance);
    void set_account_asset_balance(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t to_account_id,
        const cma_amount_t &balance);

    void deposit(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t to_account_id, const cma_amount_t &deposit);
    void withdraw(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t from_account_id,
        const cma_amount_t &withdrawal);
    void transfer(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t from_account_id,
        cma_ledger_account_id_t to_account_id, const cma_amount_t &amount);
};

#endif // CMA_LEDGER_IMPL_H
