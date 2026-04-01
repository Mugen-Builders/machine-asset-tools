
#include <concepts>
// #include <cstdint>
#include <cstring>
#include <exception>
#include <format>
#include <iostream>
#include <cmath> // For std::log2
#include <iomanip>
// #include <memory>
// #include <ranges>
// #include <span>
// #include <vector>

#include "complete-merkle-tree.h"
#include "hash-tree-proof.h"
#include "keccak-256-hasher.h"
#include "machine-hash.h"
#include "concepts.h"

// #include "ledger.h" // implicit
// #include "types.h" // implicit
#include "interprocess.hpp"
#include "ledger_impl.h"

using cartesi::complete_merkle_tree;
using cartesi::hash_tree_proof;
using cartesi::keccak_256_hasher;
using cartesi::machine_hash;


// using StateFile;

static constexpr int ACCOUNT_SIZE = sizeof(cma_ledger_account_balance_t);
static constexpr int WORD_SIZE = 5;


using AccountBytes = std::span<const uint8_t, sizeof(cma_ledger_account_balance_t)>;


namespace {

static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__, "code assumes little-endian byte ordering");

template <cartesi::ContiguousRangeOfByteLike R>
    requires std::ranges::sized_range<R>
static inline std::string to_hex(const R &r) {
    static constexpr char hex_chars[] = "0123456789abcdef";
    auto n = std::size(r);
    std::string s;
    s.resize_and_overwrite(2 * n + 2, [&](char *buf, size_t) {
        buf[0] = '0';
        buf[1] = 'x';
        for (size_t i = 0; i < n; ++i) {
            buf[2 + i * 2] = hex_chars[r[i] >> 4];
            buf[2 + i * 2 + 1] = hex_chars[r[i] & 0x0F];
        }
        return 2 * n + 2;
    });
    return s;
}


std::string address_to_string(const cma_abi_address_t &addr) {
    return to_hex(addr.data);
}

std::string b32_to_string(const cma_token_id_t &val) {
    return to_hex(val.data);
}

std::string int_to_hex(int value) {
    std::stringstream ss;
    ss << "0x" << std::hex<< std::setw(8) << std::setfill('0') << value;
    return ss.str();
}

auto address_from_string(std::string_view str, cma_abi_address_t &addr) -> void {
    if (!str.starts_with("0x") && !str.starts_with("0X")) {
        throw std::invalid_argument{std::format("missing 0x prefix in address '{}'", str)};
    }
    auto body = str.substr(2);
    if (body.size() > 2 * CMA_ABI_ADDRESS_LENGTH) {
        throw std::invalid_argument{std::format("address '{}' is too long", str)};
    }
    const unsigned nbytes = (body.size() + 1) / 2;
    unsigned hex = 0;
    // if size is odd, do first byte using a single nibble
    if (body.size() % 2 != 0) {
        auto [conv_endr, conv_ec] =
            // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage,-warnings-as-errors,cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
            std::from_chars(body.data(), body.data() + 1, addr.data[CMA_ABI_ADDRESS_LENGTH - nbytes], 16);
        if (conv_ec != std::errc{}) {
            throw std::invalid_argument{std::format("invalid hex character in address '{}'", str)};
        }
        ++hex;
    }
    for (unsigned byte = body.size() % 2; hex < body.size(); hex += 2, ++byte) {
        auto [conv_endr, conv_ec] =
            // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage,-warnings-as-errors,cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
            std::from_chars(body.data() + hex, body.data() + hex + 2, addr.data[20 - nbytes + byte], 16);
        if (conv_ec != std::errc{}) {
            throw std::invalid_argument{std::format("invalid hex character in address '{}'", str)};
        }
    }
}

// cma_amount_t value_from_string(std::string_view str) {
//     cma_amount_t amount{};
//     // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
//     auto [end, ec] = std::from_chars(str.data(), str.data() + str.size(), amount);
//     if (ec == std::errc::invalid_argument) {
//         throw std::invalid_argument{std::format("value '{}' is not a number", str)};
//     }
//     if (ec == std::errc::result_out_of_range) {
//         throw std::out_of_range{std::format("value '{}' is out of range", str)};
//     }
//     // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
//     if (end != str.data() + str.size()) {
//         throw std::runtime_error{std::format("trailing characters after value in '{}'", str)};
//     }
//     return amount;
// }

auto id_from_string(std::string_view str, cma_token_id_t token_id) -> void {
    if (!str.starts_with("0x") && !str.starts_with("0X")) {
        throw std::invalid_argument{std::format("missing 0x prefix in value '{}'", str)};
    }
    auto body = str.substr(2);
    if (body.size() > 2 * CMA_ABI_U256_LENGTH) {
        throw std::invalid_argument{std::format("value '{}' is too long", str)};
    }
    const unsigned nbytes = (body.size() + 1) / 2;
    unsigned hex = 0;
    // if size is odd, do first byte using a single nibble
    if (body.size() % 2 != 0) {
        auto [conv_endr, conv_ec] =
            // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage,-warnings-as-errors,cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
            std::from_chars(body.data(), body.data() + 1, token_id.data[CMA_ABI_ID_LENGTH - nbytes], 16);
        if (conv_ec != std::errc{}) {
            throw std::invalid_argument{std::format("invalid hex character in token_id '{}'", str)};
        }
        ++hex;
    }
    for (unsigned byte = body.size() % 2; hex < body.size(); hex += 2, ++byte) {
        auto [conv_endr, conv_ec] =
            // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage,-warnings-as-errors,cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
            std::from_chars(body.data() + hex, body.data() + hex + 2, token_id.data[CMA_ABI_ID_LENGTH - nbytes + byte], 16);
        if (conv_ec != std::errc{}) {
            throw std::invalid_argument{std::format("invalid hex character in token_id '{}'", str)};
        }
    }
}

size_t value_from_string(std::string_view str) {
    size_t amount;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto [end, ec] = std::from_chars(str.data(), str.data() + str.size(), amount);
    if (ec == std::errc::invalid_argument) {
        throw std::invalid_argument{std::format("value '{}' is not a number", str)};
    }
    if (ec == std::errc::result_out_of_range) {
        throw std::out_of_range{std::format("value '{}' is out of range", str)};
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (end != str.data() + str.size()) {
        throw std::runtime_error{std::format("trailing characters after value in '{}'", str)};
    }
    return amount;
}



void dump(cma_ledger_memory &ledger, size_t ledger_offset) {
    std::cout << "[";
    bool first = true;
    uint64_t offset = 0; // dunno why std::views::enumerate is not found...
    auto balances = ledger.get_balances();
    if (balances == nullptr) {
        throw std::runtime_error{"Invalid balances"};
    }
    char *mem_addr = reinterpret_cast<char *>(ledger.get_memory_address());
    char *bal_addr = reinterpret_cast<char *>(balances->data());

    for (size_t i = 0; i < balances->size(); ++i) {
        const auto balance = (*balances)[i];
        if (first) {
            first = false;
        } else {
            std::cout << ",";
        }
        std::cout << "\n  {\n";
        std::cout << "    \"type\": " << balance.type << ",\n";
        std::cout << R"(    "owner": )" << address_to_string(balance.owner) << '"' << ",\n";
        if (balance.type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS || balance.type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID){
            std::cout << R"(    "token_address": )" << address_to_string(balance.token_address) << '"' << ",\n";
        }
        if (balance.type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID){
            std::cout << R"(    "token_id": )" << b32_to_string(balance.token_id) << '"' << ",\n";
        }
        std::cout << R"(    "amount": )" << b32_to_string(balance.amount) << '"' << ",\n";
        std::cout << "    \"index\": " << i << ",\n";
        std::cout << "    \"drive_offset\": " << int_to_hex(ledger_offset + bal_addr - mem_addr + offset) << "\n";
        std::cout << "  }";
        offset += ACCOUNT_SIZE;
    }
    std::cout << "\n]\n";
}

void dump(const cma_ledger_account_balance_info_t &account_balance_info, size_t ledger_offset) {
    std::cout << "{\n";
    std::cout << "  \"type\": " << account_balance_info.balance->type << ",\n";
    std::cout << R"(  "owner": )" << address_to_string(account_balance_info.balance->owner) << '"' << ",\n";
    if (account_balance_info.balance->type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS || account_balance_info.balance->type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID){
        std::cout << R"(  "token_address": )" << address_to_string(account_balance_info.balance->token_address) << '"' << ",\n";
    }
    if (account_balance_info.balance->type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID){
        std::cout << R"(  "token_id": )" << b32_to_string(account_balance_info.balance->token_id) << '"' << ",\n";
    }
    std::cout << R"(  "amount": )" << b32_to_string(account_balance_info.balance->amount) << '"' << ",\n";
    std::cout << "  \"index\": " << account_balance_info.index << ",\n";
    std::cout << "  \"drive_offset\": " << int_to_hex(account_balance_info.offset + ledger_offset) << "\n";
    std::cout << "}\n";
}

void dump(const cma_ledger_account_balance_t &balance, size_t index, size_t drive_offset) {
    std::cout << "{\n";
    std::cout << "  \"type\": " << balance.type << ",\n";
    std::cout << R"(  "owner": )" << address_to_string(balance.owner) << '"' << ",\n";
    if (balance.type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS || balance.type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID){
        std::cout << R"(  "token_address": )" << address_to_string(balance.token_address) << '"' << ",\n";
    }
    if (balance.type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID){
        std::cout << R"(  "token_id": )" << b32_to_string(balance.token_id) << '"' << ",\n";
    }
    std::cout << R"(  "amount": )" << b32_to_string(balance.amount) << '"' << ",\n";
    std::cout << "  \"index\": " << index << ",\n";
    std::cout << "  \"drive_offset\": " << int_to_hex(drive_offset) << "\n";
    std::cout << "}\n";
}

auto account_bytes(const cma_ledger_account_balance_t &account) {
    // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
    return AccountBytes{reinterpret_cast<const uint8_t *>(std::addressof(account)), sizeof(account)};
}

template <std::ranges::sized_range D>
    requires std::same_as<std::ranges::range_value_t<D>, cma_ledger_account_balance_t>
auto leaf_hashes(const D &data) {
    return data | std::views::transform([](const auto &account) -> machine_hash {
        return get_hash(keccak_256_hasher{}, account_bytes(account));
    }) | std::ranges::to<std::vector<machine_hash>>();
}

void dump(const hash_tree_proof &proof, const cma_ledger_account_balance_t &balance, size_t drive_offset) {
    std::cout << "{\n";
    std::cout << "  \"log2_root_size\": " << proof.get_log2_root_size() << ",\n";
    std::cout << R"(  "root_hash: ")" << to_hex(proof.get_root_hash()) << '"' << ",\n";
    std::cout << R"(  "target_address": )" << proof.get_target_address() << ",\n";
    std::cout << R"(  "log2_target_size": )" << proof.get_log2_target_size() << ",\n";
    std::cout << R"(  "target_hash": ")" << to_hex(proof.get_target_hash()) << '"' << ",\n";
    std::cout << R"(  "target_data": ")" << to_hex(account_bytes(balance)) << '"' << ",\n";
    std::cout << R"(  "sibling_hashes": [)" << '\n';
    for (auto i = proof.get_log2_target_size(); i < proof.get_log2_root_size(); ++i) {
        std::cout << R"(    ")" << to_hex(proof.get_sibling_hash(i)) << '"';
        if (i < proof.get_log2_root_size() - 1) {
            std::cout << ',';
        }
        std::cout << '\n';
    }
    std::cout << "  ],\n";
    std::cout << "  \"drive_offset\": " << int_to_hex(drive_offset) << "\n";
    std::cout << "}\n";
}

void help() {
    std::cout << R"(account-driver-reader [options] <drive-filename> [balance params]
  inspect the ledger in the drive file pointed to by <state-filename>
  if not printing totla, nor dumping all balances, you should set balance params
  otherwise, the file is opened for modification/inspection
balance params are
  owner address: required
  token address: if asset has token address
  token id:      if asset has token id
options are
  --ledger-offset <offset>
    sets the offset of the ledger in the drive file
  --mem-length <size of the memory file>
    sets the size of the memory file
  --n-accounts <number of accounts>
    sets the maximum number of accounts in the ledger
  --n-assets <number of assets>
    sets the maximum number of assets in the ledger
  --n-balances <number of balances>
    sets the maximum number of balances in the ledger
  --total
    prints the number of balances to stdout
  --balance-index <index>
    get the balance at this index
  --dump
    dumps all accounts into a JSON array and sends it to stdout
  --dump-proof
    dumps a Merkle proof for the value of the account into a JSON object and sends it to stdout
    e.g., to dump a proof for the account of 0x0000000000000000000000000000000000000001
      ./ew state --dump-proof 0x1
          {
            "log2_root_size": 22,
            "root_hash": "0xcaf41ae20ebfa2973b1ead4bb6318d7b11591852ac925d668dfdf85a7efadb88",
            "target_address": 0,
            "log2_target_size": 5,
            "target_hash": "0xb3eb0e4ef0f57d13c2d25def39b2231978f9505b2b8c6f9dc1365d82e2371835",
            "target_data": "0x0a00000000000000000000000000000000000000000000000000000100000000",
            "sibling_hashes": [
              "0xbf7df83ae7ba62315a09bf233febe5518c4a107f7433ac02f9f87d695abc3e5f",
              "0x633dc4d7da7256660a892f8f1604a44b5432649cc8ec5cb3ced4c4e6ac94dd1d",
              "0x890740a8eb06ce9be422cb8da5cdafc2b58c0a5e24036c578de2a433c828ff7d",
              "0x3b8ec09e026fdc305365dfc94e189a81b38c7597b3d941c279f042e8206e0bd8",
              "0xecd50eee38e386bd62be9bedb990706951b65fe053bd9d8a521af753d139e2da",
              "0xdefff6d330bb5403f63b14f33b578274160de3a50df4efecf0e0db73bcdd3da5",
              "0x617bdd11f7c0a11f49db22f629387a12da7596f9d1704d7465177c63d88ec7d7",
              "0x292c23a9aa1d8bea7e2435e555a4a60e379a5a35f3f452bae60121073fb6eead",
              "0xe1cea92ed99acdcb045a6726b2f87107e8a61620a232cf4d7d5b5766b3952e10",
              "0x7ad66c0a68c72cb89e4fb4303841966e4062a76ab97451e3b9fb526a5ceb7f82",
              "0xe026cc5a4aed3c22a58cbd3d2ac754c9352c5436f638042dca99034e83636516",
              "0x3d04cffd8b46a874edf5cfae63077de85f849a660426697b06a829c70dd1409c",
              "0xad676aa337a485e4728a0b240d92b3ef7b3c372d06d189322bfd5f61f1e7203e",
              "0xa2fca4a49658f9fab7aa63289c91b7c7b6c832a6d0e69334ff5b0a3483d09dab",
              "0x4ebfd9cd7bca2505f7bef59cc1c12ecc708fff26ae4af19abe852afe9e20c862",
              "0x2def10d13dd169f550f578bda343d9717a138562e0093b380a1120789d53cf10",
              "0x776a31db34a1a0a7caaf862cffdfff1789297ffadc380bd3d39281d340abd3ad"
            ]
          }
  --help
    prints this help message
)";
}

typedef enum : uint8_t {
    NOP,
    DUMP,
    TOTAL,
    FIND_INDEX,
    FIND_BASE,
    FIND_TOKEN_ADDRESS,
    FIND_TOKEN_ADDRESS_ID,
} drive_operation_t;


} // anonymous namespace

auto main(int argc, const char *argv[]) -> int try {
    // static_assert(ACCOUNT_SIZE == WORD_SIZE,
    //     "unexpected account size (expected 32 bytes, same as machine word size)");
    std::string memory_file_name;
    size_t mem_length = 64UL * 1024 * 1024;
    size_t n_accounts = 16UL * 1024;
    size_t n_assets = 8UL;
    size_t n_balances = n_assets * n_accounts;
    size_t ledger_offset = 0;
    size_t balance_index = 0;
    cma_abi_address_t owner_address;
    cma_token_address_t token_address;
    cma_token_id_t token_id;

    drive_operation_t operation = NOP;
    bool dump_proof = false;

    for (int i = 1; i < argc; ++i) {
        // NOLINTBEGIN (cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (strcmp(argv[i], "--ledger-offset") == 0) {
            if (i + 1 >= argc) {
                throw std::runtime_error{"missing ledger offset in flie"};
            }
            ledger_offset = value_from_string(argv[i + 1]);
            ++i;
        } else if (strcmp(argv[i], "--mem-length") == 0) {
            if (i + 1 >= argc) {
                throw std::runtime_error{"missing mem length"};
            }
            mem_length = value_from_string(argv[i + 1]);
            ++i;
        } else if (strcmp(argv[i], "--n-accounts") == 0) {
            if (i + 1 >= argc) {
                throw std::runtime_error{"missing n accounts"};
            }
            n_accounts = value_from_string(argv[i + 1]);
            ++i;
        } else if (strcmp(argv[i], "--n-assets") == 0) {
            if (i + 1 >= argc) {
                throw std::runtime_error{"missing n assets"};
            }
            n_assets = value_from_string(argv[i + 1]);
            ++i;
        } else if (strcmp(argv[i], "--n-balances") == 0) {
            if (i + 1 >= argc) {
                throw std::runtime_error{"missing n balances"};
            }
            n_balances = value_from_string(argv[i + 1]);
            ++i;
        } else if (strcmp(argv[i], "--balance-index") == 0) {
            if (i + 1 >= argc) {
                throw std::runtime_error{"missing balance index"};
            }
            balance_index = value_from_string(argv[i + 1]);
            operation = FIND_INDEX;
            ++i;
        } else if (strcmp(argv[i], "--dump-proof") == 0) {
            dump_proof = true;
        } else if (strcmp(argv[i], "--dump") == 0) {
            operation = DUMP;
        } else if (strcmp(argv[i], "--total") == 0) {
            operation = TOTAL;
        } else if (strcmp(argv[i], "--help") == 0) {
            help();
            return 0;
        } else {
            if (i + 1 > argc) {
                throw std::runtime_error{"missing drive information"};
            }
            memory_file_name = argv[i];
            if (operation != NOP) {
                if (argc > i + 1) {
                    throw std::runtime_error{std::format("unknown values for operation")};
                }
            } else {
                switch (argc - i) {
                    case 2:
                        operation = FIND_BASE;
                        address_from_string(argv[i + 1], owner_address);
                        break;
                    case 3:
                        operation = FIND_TOKEN_ADDRESS;
                        address_from_string(argv[i + 1], owner_address);
                        address_from_string(argv[i + 2], token_address);
                        break;
                    case 4:
                        operation = FIND_TOKEN_ADDRESS_ID;
                        address_from_string(argv[i + 1], owner_address);
                        address_from_string(argv[i + 2], token_address);
                        id_from_string(argv[i + 3], token_id);
                        break;
                    default:
                        throw std::runtime_error{"Cannot detect operation"};
                }
            }
            break;
        }
        // NOLINTEND (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    if (operation == NOP) {
        throw std::runtime_error{"nothing to do"};
    }

    cma_ledger_memory ledger(interprocess::open_only, memory_file_name.c_str(), ledger_offset, mem_length, n_accounts,
        n_assets, n_balances);

    if (operation == DUMP) {
        dump(ledger, ledger_offset);
        return 0;
    }
    if (operation == TOTAL) {
        auto balances = ledger.get_balances();
        if (balances == nullptr) {
            throw std::runtime_error{"Invalid balances"};
        }
        std::cout << balances->size() << '\n';
        return 0;
    }
    if (operation == FIND_INDEX) {
        auto balances = ledger.get_balances();
        if (balances == nullptr) {
            throw std::runtime_error{"Invalid balances"};
        }
        auto balance = (*balances)[balance_index];
        if (balance.type == 0) {
            throw std::runtime_error{"Invalid balance type"};
        }
        char *mem_addr = reinterpret_cast<char *>(ledger.get_memory_address());
        char *bal_addr = reinterpret_cast<char *>(balances->data());
        size_t drive_offset = ledger_offset + bal_addr - mem_addr;
        if (dump_proof) {
            size_t log2_max_accounts = (n_balances < 1) ? 0 : std::bit_width(n_balances - 1);
            size_t log2_account_size = static_cast<size_t>(std::log2(static_cast<double>(sizeof(cma_ledger_account_balance_t))));

            const auto log2_root_size = log2_max_accounts + log2_account_size;
            const auto log2_leaf_size = log2_account_size;
            const auto log2_word_size = WORD_SIZE;
            std::unique_ptr<complete_merkle_tree> mt = std::make_unique<complete_merkle_tree>(log2_root_size, log2_leaf_size, log2_word_size,
                    leaf_hashes(balances[0]));
            dump(mt->get_proof(balance_index * ACCOUNT_SIZE, log2_leaf_size), balance, drive_offset);
        } else {
            dump(balance, balance_index, drive_offset);
        }
        return 0;
    }
    cma_ledger_account_type_t account_type = CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS;
    cma_ledger_account_id_t account_id;
    ledger.retrieve_account(&account_id, nullptr, &owner_address, nullptr, account_type, CMA_LEDGER_OP_FIND);

    cma_ledger_asset_id_t asset_id;
    cma_ledger_asset_type_t asset_type;
    cma_ledger_account_balance_info_t account_balance_info;
    switch (operation) {
        case FIND_BASE:
            asset_type = CMA_LEDGER_ASSET_TYPE_BASE;
            ledger.retrieve_asset(&asset_id, nullptr, nullptr, nullptr, asset_type, CMA_LEDGER_OP_FIND);
            ledger.get_account_asset_balance(asset_id, account_id, nullptr, &account_balance_info);
            break;
        case FIND_TOKEN_ADDRESS:
            asset_type = CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS;
            ledger.retrieve_asset(&asset_id, &token_address, nullptr, nullptr, asset_type, CMA_LEDGER_OP_FIND);
            ledger.get_account_asset_balance(asset_id, account_id, nullptr, &account_balance_info);
            break;
        case FIND_TOKEN_ADDRESS_ID:
            asset_type = CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID;
            ledger.retrieve_asset(&asset_id, &token_address, &token_id, nullptr, asset_type, CMA_LEDGER_OP_FIND);
            ledger.get_account_asset_balance(asset_id, account_id, nullptr, &account_balance_info);
            break;
        default:
            throw std::runtime_error{"Unknown operation"};
    }

    if (account_balance_info.balance == nullptr) {
        throw std::runtime_error{std::format("user '{}' has no base asset balance",address_to_string(owner_address))};
    }
    if (dump_proof) {

        auto balances = ledger.get_balances();
        if (balances == nullptr) {
            throw std::runtime_error{"Invalid balances"};
        }
        char *mem_addr = reinterpret_cast<char *>(ledger.get_memory_address());
        char *bal_addr = reinterpret_cast<char *>(balances->data());

        size_t log2_max_accounts = (n_balances < 1) ? 0 : std::bit_width(n_balances - 1);
        size_t log2_account_size = static_cast<size_t>(std::log2(static_cast<double>(sizeof(cma_ledger_account_balance_t))));

        auto index = account_balance_info.index;
        const auto log2_root_size = log2_max_accounts + log2_account_size;
        const auto log2_leaf_size = log2_account_size;
        const auto log2_word_size = WORD_SIZE;
        std::unique_ptr<complete_merkle_tree> mt = std::make_unique<complete_merkle_tree>(log2_root_size, log2_leaf_size, log2_word_size,
                leaf_hashes(balances[0]));
        dump(mt->get_proof(index * ACCOUNT_SIZE, log2_leaf_size), *(account_balance_info.balance), ledger_offset + bal_addr - mem_addr);
    } else {
        dump(account_balance_info, ledger_offset);
    }

    return 0;
} catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
}
