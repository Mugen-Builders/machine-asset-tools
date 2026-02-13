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

class cma_ledger {
private:
    uint64_t magic = CMA_LEDGER_MAGIC;

public:
    ~cma_ledger();

    [[nodiscard]] auto is_initialized() const -> bool;

    virtual void clear() = 0;

    virtual auto get_asset_count() -> size_t = 0;
    virtual void retrieve_create_asset(cma_ledger_asset_id_t *asset_id, cma_token_address_t *token_address,
        cma_token_id_t *token_id, cma_ledger_asset_type_t &asset_type, cma_ledger_retrieve_operation_t operation) = 0;
    virtual auto find_asset(cma_ledger_asset_id_t asset_id, cma_ledger_asset_type_t *asset_type,
        cma_token_address_t *token_address, cma_token_id_t *token_id, cma_amount_t *supply) -> bool = 0;

    virtual auto get_account_count() -> size_t = 0;
    virtual auto find_account(cma_ledger_account_id_t account_id, cma_ledger_account_t *account) -> bool = 0;
    virtual void retrieve_create_account(cma_ledger_account_id_t *account_id, cma_ledger_account_t *account,
        const void *addr_accid, cma_ledger_account_type_t &account_type, cma_ledger_retrieve_operation_t operation) = 0;

    virtual void set_asset_supply(cma_ledger_asset_id_t asset_id, cma_amount_t &supply) = 0;
    virtual void get_account_asset_balance(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t to_account_id,
        cma_amount_t &balance) = 0;
    virtual void set_account_asset_balance(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t to_account_id,
        const cma_amount_t &balance) = 0;

    virtual void deposit(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t to_account_id,
        const cma_amount_t &deposit) = 0;
    virtual void withdraw(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t from_account_id,
        const cma_amount_t &withdrawal) = 0;
    virtual void transfer(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t from_account_id,
        cma_ledger_account_id_t to_account_id, const cma_amount_t &amount) = 0;
};

class cma_ledger_memory : public cma_ledger {
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
    cma_ledger_memory() = default;

    void clear() override;
    auto get_asset_count() -> size_t override;
    void retrieve_create_asset(cma_ledger_asset_id_t *asset_id, cma_token_address_t *token_address,
        cma_token_id_t *token_id, cma_ledger_asset_type_t &asset_type,
        cma_ledger_retrieve_operation_t operation) override;
    auto find_asset(cma_ledger_asset_id_t asset_id, cma_ledger_asset_type_t *asset_type,
        cma_token_address_t *token_address, cma_token_id_t *token_id, cma_amount_t *supply) -> bool override;

    auto get_account_count() -> size_t override;
    auto find_account(cma_ledger_account_id_t account_id, cma_ledger_account_t *account) -> bool override;
    void retrieve_create_account(cma_ledger_account_id_t *account_id, cma_ledger_account_t *account,
        const void *addr_accid, cma_ledger_account_type_t &account_type,
        cma_ledger_retrieve_operation_t operation) override;

    void set_asset_supply(cma_ledger_asset_id_t asset_id, cma_amount_t &supply) override;
    void get_account_asset_balance(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t to_account_id,
        cma_amount_t &balance) override;
    void set_account_asset_balance(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t to_account_id,
        const cma_amount_t &balance) override;

    void deposit(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t to_account_id,
        const cma_amount_t &deposit) override;
    void withdraw(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t from_account_id,
        const cma_amount_t &withdrawal) override;
    void transfer(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t from_account_id,
        cma_ledger_account_id_t to_account_id, const cma_amount_t &amount) override;
};

class cma_ledger_file : public cma_ledger {
private:
    using cma_ledger_account_key_bytes_t = std::array<uint8_t, CMA_ABI_ID_LENGTH>;
    using lassid_to_asset_t = interprocess::unordered_node_map<cma_ledger_asset_id_t, cma_ledger_asset_struct_t>;
    using asset_to_lassid_t = interprocess::unordered_node_map<cma_ledger_asset_key_bytes_t, cma_ledger_asset_id_t>;
    using laccid_to_account_t = interprocess::unordered_node_map<cma_ledger_account_id_t, cma_ledger_account_t>;
    using account_to_laccid_t =
        interprocess::unordered_node_map<cma_ledger_account_key_bytes_t, cma_ledger_account_id_t>;
    using account_asset_map_t = interprocess::unordered_node_map<cma_map_key_t, cma_amount_t, hash_pair>;

    interprocess::managed_memory m_memory; ///< Mapped memory containing the whole ledger state.
    interprocess::void_allocator &m_allocator;

    lassid_to_asset_t &lassid_to_asset;
    asset_to_lassid_t &asset_to_lassid;
    laccid_to_account_t &laccid_to_account;
    account_to_laccid_t &account_to_laccid;
    account_asset_map_t &account_asset_balance;

public:
    cma_ledger_file(const char *memory_file_name,
        size_t mem_length); //, size_t n_accounts, size_t n_assets, size_t n_account_assets);

    static auto estimate_required_size(size_t n_accounts, size_t n_assets, size_t n_account_assets) -> size_t;
    void clear() override;
    auto get_asset_count() -> size_t override;
    void retrieve_create_asset(cma_ledger_asset_id_t *asset_id, cma_token_address_t *token_address,
        cma_token_id_t *token_id, cma_ledger_asset_type_t &asset_type,
        cma_ledger_retrieve_operation_t operation) override;
    auto find_asset(cma_ledger_asset_id_t asset_id, cma_ledger_asset_type_t *asset_type,
        cma_token_address_t *token_address, cma_token_id_t *token_id, cma_amount_t *supply) -> bool override;

    auto get_account_count() -> size_t override;
    auto find_account(cma_ledger_account_id_t account_id, cma_ledger_account_t *account) -> bool override;
    void retrieve_create_account(cma_ledger_account_id_t *account_id, cma_ledger_account_t *account,
        const void *addr_accid, cma_ledger_account_type_t &account_type,
        cma_ledger_retrieve_operation_t operation) override;

    void set_asset_supply(cma_ledger_asset_id_t asset_id, cma_amount_t &supply) override;
    void get_account_asset_balance(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t to_account_id,
        cma_amount_t &balance) override;
    void set_account_asset_balance(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t to_account_id,
        const cma_amount_t &balance) override;

    void deposit(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t to_account_id,
        const cma_amount_t &deposit) override;
    void withdraw(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t from_account_id,
        const cma_amount_t &withdrawal) override;
    void transfer(cma_ledger_asset_id_t asset_id, cma_ledger_account_id_t from_account_id,
        cma_ledger_account_id_t to_account_id, const cma_amount_t &amount) override;
};

#endif // CMA_LEDGER_IMPL_H
