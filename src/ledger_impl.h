#ifndef CMA_LEDGER_IMPL_H
#define CMA_LEDGER_IMPL_H

#include <array>
#include <cstddef>
#include <string> // for string class
#include <unordered_map>

extern "C" {
#include "libcma/ledger.h"
#include "libcma/types.h"
}

#include "interprocess.hpp"

namespace interprocess = libcma::interprocess;

enum : uint8_t {
    CMA_LEDGER_ASSET_TYPE_SIZE = 1,
    CMA_LEDGER_ASSET_ARRAY_KEY_TYPE_IND = 0,
    CMA_LEDGER_ASSET_ARRAY_KEY_ADDRESS_IND = CMA_LEDGER_ASSET_ARRAY_KEY_TYPE_IND + CMA_LEDGER_ASSET_TYPE_SIZE,
    CMA_LEDGER_ASSET_ARRAY_KEY_ID_IND = CMA_LEDGER_ASSET_ARRAY_KEY_ADDRESS_IND + CMA_ABI_ADDRESS_LENGTH,
    CMA_LEDGER_ASSET_MAP_KEY_SIZE = CMA_LEDGER_ASSET_TYPE_SIZE + CMA_ABI_ADDRESS_LENGTH + CMA_ABI_ID_LENGTH,
};

enum : uint64_t {
    CMA_LEDGER_MAGIC = 0x6de6c7b338afbad6,
    CMA_HASH_PAIR_CTE = 0x9e3779b9,
    CMA_HASH_PAIR_LSHIFT = 6,
    CMA_HASH_PAIR_RSHIFT = 2,
};

using cma_ledger_asset_struct_t = struct cma_ledger_asset_struct {
    cma_ledger_asset_type_t type;
    cma_token_address_t token_address;
    cma_token_id_t token_id;
    cma_amount_t supply;
};

// using cma_ledger_account_t = struct cma_ledger_account {
//     cma_ledger_account_t account;
//     cma_token_address_t token_address;
//     cma_token_id_t token_id;
//     cma_amount_t supply;
// };

struct hash_pair {
    template <class T1, class T2>
    auto operator()(const std::pair<T1, T2> &pair_ref) const -> size_t {
        const size_t hash1 = std::hash<T1>{}(pair_ref.first);
        const size_t hash2 = std::hash<T2>{}(pair_ref.second);
        return hash1 ^ (hash2 + CMA_HASH_PAIR_CTE + (hash1 << CMA_HASH_PAIR_LSHIFT) + (hash1 >> CMA_HASH_PAIR_RSHIFT));
    }
};

using cma_ledger_asset_key_bytes_t = std::array<uint8_t, CMA_LEDGER_ASSET_MAP_KEY_SIZE>;

using cma_map_key_t = std::pair<cma_ledger_account_id_t, cma_ledger_asset_id_t>;

class cma_ledger_base {
private:
    uint64_t magic = CMA_LEDGER_MAGIC;

public:
    ~cma_ledger_base();

    [[nodiscard]] auto is_initialized() const -> bool;

    virtual void clear() = 0;

    virtual auto get_asset_count() -> size_t = 0;
    virtual void retrieve_asset(cma_ledger_asset_id_t *asset_id, cma_token_address_t *token_address,
        cma_token_id_t *token_id, cma_amount_t *out_total_supply, cma_ledger_asset_type_t &asset_type,
        cma_ledger_retrieve_operation_t operation) = 0;
    virtual auto find_asset(cma_ledger_asset_id_t asset_id, cma_ledger_asset_type_t *asset_type,
        cma_token_address_t *token_address, cma_token_id_t *token_id, cma_amount_t *supply) -> bool = 0;

    virtual auto get_account_count() -> size_t = 0;
    virtual auto find_account(cma_ledger_account_id_t account_id, cma_ledger_account_t *account, size_t *n_balances)
        -> bool = 0;
    virtual void retrieve_account(cma_ledger_account_id_t *account_id, cma_ledger_account_t *account,
        const void *addr_accid, size_t *n_balances, cma_ledger_account_type_t &account_type,
        cma_ledger_retrieve_operation_t operation) = 0;

    virtual void set_asset_supply(cma_ledger_asset_id_t asset_id, cma_amount_t &supply) = 0;
    virtual void get_account_asset_balance(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t account_id,
        cma_amount_t *balance, cma_ledger_account_balance_info_t *account_balance_info) = 0;
    virtual void set_account_asset_balance(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t account_id,
        const cma_amount_t &balance) = 0;

    virtual void deposit(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t to_account_id,
        const cma_amount_t &deposit) = 0;
    virtual void withdraw(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t from_account_id,
        const cma_amount_t &withdrawal) = 0;
    virtual void transfer(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t from_account_id,
        cma_ledger_account_id_t to_account_id, const cma_amount_t &amount) = 0;
};

class cma_ledger_basic : public cma_ledger_base {
private:
    using cma_ledger_asset_key_t = std::string;
    using cma_ledger_account_key_t = std::string;
    using lassid_to_asset_t = std::unordered_map<cma_ledger_asset_id_t, cma_ledger_asset_struct_t>;
    using asset_to_lassid_t = std::unordered_map<cma_ledger_asset_key_t, cma_ledger_asset_id_t>;
    using laccid_to_account_t = std::unordered_map<cma_ledger_account_id_t, cma_ledger_account_t>;
    using account_to_laccid_t = std::unordered_map<cma_ledger_account_key_t, cma_ledger_account_id_t>;
    using account_asset_map_t = std::unordered_map<cma_map_key_t, cma_amount_t, hash_pair>;

    lassid_to_asset_t lassid_to_asset;
    asset_to_lassid_t asset_to_lassid;
    laccid_to_account_t laccid_to_account;
    account_to_laccid_t account_to_laccid;
    account_asset_map_t account_asset_balance;

public:
    cma_ledger_basic() = default;

    void clear() override;
    auto get_asset_count() -> size_t override;
    void retrieve_asset(cma_ledger_asset_id_t *asset_id, cma_token_address_t *token_address, cma_token_id_t *token_id,
        cma_amount_t *out_total_supply, cma_ledger_asset_type_t &asset_type,
        cma_ledger_retrieve_operation_t operation) override;
    auto find_asset(cma_ledger_asset_id_t asset_id, cma_ledger_asset_type_t *asset_type,
        cma_token_address_t *token_address, cma_token_id_t *token_id, cma_amount_t *supply) -> bool override;

    auto get_account_count() -> size_t override;
    auto find_account(cma_ledger_account_id_t account_id, cma_ledger_account_t *account, size_t *n_balances)
        -> bool override;
    void retrieve_account(cma_ledger_account_id_t *account_id, cma_ledger_account_t *account, const void *addr_accid,
        size_t *n_balances, cma_ledger_account_type_t &account_type,
        cma_ledger_retrieve_operation_t operation) override;

    void set_asset_supply(cma_ledger_asset_id_t asset_id, cma_amount_t &supply) override;
    void get_account_asset_balance(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t account_id,
        cma_amount_t *balance, cma_ledger_account_balance_info_t *account_balance_info) override;
    void set_account_asset_balance(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t account_id,
        const cma_amount_t &balance) override;

    void deposit(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t to_account_id,
        const cma_amount_t &deposit) override;
    void withdraw(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t from_account_id,
        const cma_amount_t &withdrawal) override;
    void transfer(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t from_account_id,
        cma_ledger_account_id_t to_account_id, const cma_amount_t &amount) override;
};

// TODO: remove extra maps
// TODO: add balance list and non withdrawable balance list
// TODO: remove account+asset pair map and use pointer to lists
// TODO: change asset id and account id to single representation

class cma_ledger_memory : public cma_ledger_base {
private:
    typedef enum {
        CMA_LEDGER_BALANCE_TYPE_VIRTUAL,
        CMA_LEDGER_BALANCE_TYPE_WITHDRAWABLE,
    } cma_balance_type_t;

    using cma_ledger_account_virtual_balance_t = struct cma_ledger_account_virtual_balance {
        cma_amount_t amount;
    };

    using cma_ledger_offset_balance_ptr_t = interprocess::offset_ptr<cma_ledger_account_balance_t>;
    using cma_ledger_offset_virtual_balance_ptr_t = interprocess::offset_ptr<cma_ledger_account_virtual_balance_t>;

    using cma_balance_t = struct cma_balance {
        cma_balance_type_t type;
        cma_ledger_offset_balance_ptr_t withdrawable_balance;
        cma_ledger_offset_virtual_balance_ptr_t virtual_balance;
    };
    using cma_ledger_offset_balance_struct_ptr_t = interprocess::offset_ptr<cma_balance_t>;
    // using cma_ledger_balance_set_t = interprocess::unordered_flat_set<cma_map_key_t>;

    using cma_ledger_account_struct_t = struct cma_ledger_account_struct {
        cma_ledger_account_t account;
        size_t n_balances;
    };

    using cma_ledger_account_key_bytes_t = std::array<uint8_t, CMA_ABI_ID_LENGTH>;
    using lassid_to_asset_t = interprocess::unordered_node_map<cma_ledger_asset_id_t, cma_ledger_asset_struct_t>;
    using asset_to_lassid_t = interprocess::unordered_node_map<cma_ledger_asset_key_bytes_t, cma_ledger_asset_id_t>;
    using laccid_to_account_t = interprocess::unordered_node_map<cma_ledger_account_id_t, cma_ledger_account_struct_t>;
    using account_to_laccid_t =
        interprocess::unordered_node_map<cma_ledger_account_key_bytes_t, cma_ledger_account_id_t>;
    using account_asset_map_t = interprocess::unordered_node_map<cma_map_key_t, cma_balance_t, hash_pair>;

    // using balance_list_t = cma_ledger_account_balance_t*;
    using virtual_balance_list_t = interprocess::vector<cma_ledger_account_virtual_balance_t>;
    using balance_key_list_t = interprocess::vector<cma_map_key_t>;

    size_t max_accounts;
    size_t max_assets;
    size_t max_balances;
    size_t mem_offset;

    interprocess::file_mapping m_file;     ///< Mapped file containing the whole ledger state.
    interprocess::mapped_region m_region;  ///< Region of the mapped file containing the ledger state.
    cma_ledger_account_balance_t* balances;
    interprocess::managed_memory m_memory; ///< Mapped memory containing the whole ledger state.
    interprocess::void_allocator &m_allocator;

    // balance_list_t &balances;
    virtual_balance_list_t &virtual_balances;
    lassid_to_asset_t &lassid_to_asset;
    asset_to_lassid_t &asset_to_lassid;
    laccid_to_account_t &laccid_to_account;
    account_to_laccid_t &account_to_laccid;
    account_asset_map_t &account_asset_balance;
    balance_key_list_t &last_balances;
    balance_key_list_t &last_virtual_balances;
    cma_ledger_asset_id_t &next_asset_id;
    cma_ledger_account_id_t &next_account_id;
    cma_ledger_asset_id_t &base_asset_id;
    bool &base_asset_id_defined;

public:
    cma_ledger_memory(interprocess::open_only_t mode, const char *memory_file_name, size_t offset, size_t mem_length,
        size_t n_accounts, size_t n_assets, size_t n_balances);
    cma_ledger_memory(interprocess::create_only_t mode, const char *memory_file_name, size_t offset, size_t mem_length,
        size_t n_accounts, size_t n_assets, size_t n_balances);
    cma_ledger_memory(void *mem_ptr, size_t mem_length, size_t n_accounts, size_t n_assets, size_t n_balances);

    static auto estimate_required_size(size_t n_accounts, size_t n_assets, size_t n_balances) -> size_t;
    void clear() override;
    auto get_asset_count() -> size_t override;
    void retrieve_asset(cma_ledger_asset_id_t *asset_id, cma_token_address_t *token_address, cma_token_id_t *token_id,
        cma_amount_t *out_total_supply, cma_ledger_asset_type_t &asset_type,
        cma_ledger_retrieve_operation_t operation) override;
    auto find_asset(cma_ledger_asset_id_t asset_id, cma_ledger_asset_type_t *asset_type,
        cma_token_address_t *token_address, cma_token_id_t *token_id, cma_amount_t *supply) -> bool override;
    auto remove_asset(cma_ledger_asset_id_t asset_id) -> void;

    auto get_account_count() -> size_t override;
    auto find_account(cma_ledger_account_id_t account_id, cma_ledger_account_t *account, size_t *n_balances)
        -> bool override;
    void retrieve_account(cma_ledger_account_id_t *account_id, cma_ledger_account_t *account, const void *addr_accid,
        size_t *n_balances, cma_ledger_account_type_t &account_type,
        cma_ledger_retrieve_operation_t operation) override;
    auto remove_account(cma_ledger_account_id_t account_id) -> void;

    void set_asset_supply(cma_ledger_asset_id_t asset_id, cma_amount_t &supply) override;
    void get_account_asset_balance(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t account_id,
        cma_amount_t *balance, cma_ledger_account_balance_info_t *account_balance_info) override;
    void set_account_asset_balance(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t account_id,
        const cma_amount_t &balance) override;

    void deposit(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t to_account_id,
        const cma_amount_t &deposit) override;
    void withdraw(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t from_account_id,
        const cma_amount_t &withdrawal) override;
    void transfer(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t from_account_id,
        cma_ledger_account_id_t to_account_id, const cma_amount_t &amount) override;

    auto get_balances() -> cma_ledger_account_balance_t *;
    auto get_balances_size() -> size_t;
    auto get_mem_offset() -> size_t;
};

#endif // CMA_LEDGER_IMPL_H
