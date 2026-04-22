
#include <concepts>
// #include <cstdint>
#include <cstring>
#include <exception>
#include <format>
#include <iostream>
#include <cmath> // For std::log2
#include <iomanip>
#include <filesystem>
#include <fstream>
// #include <memory>
// #include <ranges>
// #include <span>
// #include <vector>
#include <cartesi-machine/machine-c-api.h>

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
static constexpr int LOG2_WORD_SIZE = 5;
static constexpr int WORD_SIZE = 32;


using  partial_ledger_account_balance_t = struct {
    uint8_t data[WORD_SIZE];
};

using AccountBytes = std::span<const uint8_t, sizeof(cma_ledger_account_balance_t)>;
using AccountBytesPartial = std::span<const uint8_t, sizeof(partial_ledger_account_balance_t)>;


namespace {

static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__, "code assumes little-endian byte ordering");

// typedef struct partial_ledger_account_balance {
//     uint8_t data[WORD_SIZE];
// } partial_ledger_account_balance_t;

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

auto b32_from_string(std::string_view str, cma_token_id_t token_id) -> void {
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
    uint8_t base = 10;
    auto body = str;
    if (str.starts_with("0x") || str.starts_with("0X")) {
        base = 16;
        body = str.substr(2);
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto [end, ec] = std::from_chars(body.data(), body.data() + body.size(), amount, base);
    if (ec == std::errc::invalid_argument) {
        throw std::invalid_argument{std::format("value '{}' is not a number", body)};
    }
    if (ec == std::errc::result_out_of_range) {
        throw std::out_of_range{std::format("value '{}' is out of range", body)};
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (end != body.data() + body.size()) {
        throw std::runtime_error{std::format("trailing characters after value in '{}'", body)};
    }
    return amount;
}

void dump(cma_ledger_memory &ledger) {
    std::cout << "[";
    bool first = true;
    uint64_t offset = 0;
    auto balances = ledger.get_balances();
    if (balances == nullptr) {
        throw std::runtime_error{"Invalid balances"};
    }
    auto mem_offset = ledger.get_mem_offset();

    for (size_t i = 0; i < ledger.get_balances_size(); ++i) {
        const auto balance = balances[i];
        if (first) {
            first = false;
        } else {
            std::cout << ",";
        }
        std::cout << "\n  {\n";
        std::cout << "    \"type\": " << balance.type << ",\n";
        std::cout << R"(    "owner": )" << address_to_string(balance.owner) << '"' << ",\n";
        if (balance.type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS || balance.type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID || balance.type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID_AMOUNT){
            std::cout << R"(    "token_address": )" << address_to_string(balance.token_address) << '"' << ",\n";
        }
        if (balance.type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID || balance.type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID_AMOUNT){
            std::cout << R"(    "token_id": )" << b32_to_string(balance.token_id) << '"' << ",\n";
        }
        std::cout << R"(    "amount": )" << b32_to_string(balance.amount) << '"' << ",\n";
        std::cout << "    \"index\": " << i << ",\n";
        std::cout << "    \"drive_offset\": " << int_to_hex(mem_offset + offset) << "\n";
        std::cout << "  }";
        offset += ACCOUNT_SIZE;
    }
    std::cout << "\n]\n";
}

void dump(const cma_ledger_account_balance_info_t &account_balance_info) {
    std::cout << "{\n";
    std::cout << "  \"type\": " << account_balance_info.balance->type << ",\n";
    std::cout << R"(  "owner": )" << address_to_string(account_balance_info.balance->owner) << '"' << ",\n";
    if (account_balance_info.balance->type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS || account_balance_info.balance->type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID || account_balance_info.balance->type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID_AMOUNT){
        std::cout << R"(  "token_address": )" << address_to_string(account_balance_info.balance->token_address) << '"' << ",\n";
    }
    if (account_balance_info.balance->type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID || account_balance_info.balance->type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID_AMOUNT){
        std::cout << R"(  "token_id": )" << b32_to_string(account_balance_info.balance->token_id) << '"' << ",\n";
    }
    std::cout << R"(  "amount": )" << b32_to_string(account_balance_info.balance->amount) << '"' << ",\n";
    std::cout << "  \"index\": " << account_balance_info.index << ",\n";
    std::cout << "  \"drive_offset\": " << int_to_hex(account_balance_info.offset) << "\n";
    std::cout << "}\n";
}

void dump(const cma_ledger_account_balance_t &balance, size_t index, size_t drive_offset) {
    std::cout << "{\n";
    std::cout << "  \"type\": " << balance.type << ",\n";
    std::cout << R"(  "owner": )" << address_to_string(balance.owner) << '"' << ",\n";
    if (balance.type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS || balance.type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID || balance.type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID_AMOUNT){
        std::cout << R"(  "token_address": )" << address_to_string(balance.token_address) << '"' << ",\n";
    }
    if (balance.type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID || balance.type == CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID_AMOUNT){
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

auto account_bytes(const partial_ledger_account_balance_t &account) {
    // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
    return AccountBytesPartial{reinterpret_cast<const uint8_t *>(std::addressof(account)), sizeof(account)};
}

template <std::ranges::sized_range D>
    requires std::same_as<std::ranges::range_value_t<D>, partial_ledger_account_balance_t>
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
  if searching for accounts, you should set balance params
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
  --initialize
    initialize a new drive file
  --balance-index <index>
    get the balance at this index
  --dump
    dumps all accounts into a JSON array and sends it to stdout
  --dump-proof
    dumps a Merkle proof for the value of the account into a JSON object and sends it to stdout
  --dump-full-proof <snapshot path> <account drive start>
    dumps a Merkle proof up to the machine root for the value of the account into a JSON object and sends it to stdout
  --dump-custom-full-proof <snapshot path> <start> <size>
    dumps a Merkle proof up to the machine root for the data in <start> and <size> into a JSON object and sends it to stdout
  --help
    prints this help message
)";
}

typedef enum : uint8_t {
    NOP,
    DUMP,
    TOTAL,
    INITIALIZE_DRIVE,
    FIND_INDEX,
    FIND_BASE,
    FIND_TOKEN_ADDRESS,
    FIND_TOKEN_ADDRESS_ID,
} drive_operation_t;


} // anonymous namespace

auto main(int argc, const char *argv[]) -> int try {
    // static_assert(ACCOUNT_SIZE == LOG2_WORD_SIZE,
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
    std::string machine_snapshot_path;
    size_t flash_drive_offset = 0;

    drive_operation_t operation = NOP;
    bool dump_proof = false;
    bool dump_full_proof = false;

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
        } else if (strcmp(argv[i], "--total") == 0) {
            operation = TOTAL;
        } else if (strcmp(argv[i], "--dump") == 0) {
            operation = DUMP;
        } else if (strcmp(argv[i], "--dump-proof") == 0) {
            dump_proof = true;
        } else if (strcmp(argv[i], "--initialize") == 0) {
            operation = INITIALIZE_DRIVE;
        } else if (strcmp(argv[i], "--dump-full-proof") == 0) {
            if (i + 2 >= argc) {
                throw std::runtime_error{"missing machine snapshot path"};
            }
            machine_snapshot_path = argv[i+1];
            flash_drive_offset = value_from_string(argv[i + 2]);
            dump_full_proof = true;
            i += 2;
        } else if (strcmp(argv[i], "--dump-custom-full-proof") == 0) {
            if (i + 3 >= argc) {
                throw std::runtime_error{"missing machine snapshot path"};
            }
            std::string snapshot_path = argv[i+1];
            size_t offset = value_from_string(argv[i + 2]);
            size_t data_size = value_from_string(argv[i + 3]);

            std::string config_path = snapshot_path + "/config.json";
            if (!std::filesystem::exists(snapshot_path) || !std::filesystem::exists(config_path)) {
                throw std::runtime_error{std::format("coundn'l find snapshot config '{}' ",config_path)};
            }

            std::ifstream config_file(config_path);
            if (!config_file.is_open()) {
                throw std::runtime_error{std::format("unable to open config '{}'",config_path)};
            }
            std::string config_json;
            std::string line;
            while (std::getline(config_file, line)) {
                config_json += line;
            }

            cm_machine *machine = nullptr;
            // if (cm_load_new(machine_snapshot_path.c_str(), config_json.c_str(), CM_SHARING_NONE, &machine) != CM_ERROR_OK) { // 0.20.0
            if (cm_load_new(snapshot_path.c_str(), config_json.c_str(), &machine) != CM_ERROR_OK) {
              throw std::runtime_error{std::format("failed to load machine: {}", cm_get_last_error_message())};
            }
            const char *proof;
            size_t log2_data_size = static_cast<size_t>(std::log2(static_cast<double>(data_size)));

            if (cm_get_proof(machine, offset, log2_data_size, &proof) != CM_ERROR_OK) {
              throw std::runtime_error{std::format("couldn't get proof: {}", cm_get_last_error_message())};
            }
            std::cout << proof << '\n';
            return 0;
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
                        b32_from_string(argv[i + 3], token_id);
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

    if (operation == INITIALIZE_DRIVE) {
        std::ofstream memfile(memory_file_name, std::ios::binary | std::ios::out);
        if (ledger_offset + mem_length > 0) {
            memfile.seekp(ledger_offset + mem_length - 1);
            memfile.put('\0');
        }
        memfile.close();
        cma_ledger_memory ledger(interprocess::create_only, memory_file_name.c_str(), ledger_offset, mem_length, n_accounts,
            n_assets, n_balances);
        return 0;
    }

    cma_ledger_memory ledger(interprocess::open_only, memory_file_name.c_str(), ledger_offset, mem_length, n_accounts,
        n_assets, n_balances);

    if (operation == DUMP) {
        dump(ledger);
        return 0;
    }
    if (operation == TOTAL) {
        std::cout << ledger.get_balances_size() << '\n';
        return 0;
    }
    if (operation == FIND_INDEX) {
        auto balances = ledger.get_balances();
        if (balances == nullptr) {
            throw std::runtime_error{"Invalid balances"};
        }
        auto balance = balances[balance_index];
        if (balance.type == 0) {
            throw std::runtime_error{"Invalid balance type"};
        }
        auto mem_offset = ledger.get_mem_offset();
        size_t drive_offset = mem_offset + balance_index * ACCOUNT_SIZE;
        if (dump_proof) {
            size_t log2_max_accounts = (n_balances < 1) ? 0 : std::bit_width(n_balances - 1);
            size_t log2_account_size = static_cast<size_t>(std::log2(static_cast<double>(ACCOUNT_SIZE)));

            partial_ledger_account_balance_t *partial_balances = reinterpret_cast<partial_ledger_account_balance_t *>(balances);

            const auto log2_root_size = log2_max_accounts + log2_account_size;
            const auto log2_leaf_size = log2_account_size;
            const auto log2_word_size = LOG2_WORD_SIZE;
            std::unique_ptr<complete_merkle_tree> mt = std::make_unique<complete_merkle_tree>(
                log2_root_size, log2_word_size, log2_word_size,
                leaf_hashes(std::span<partial_ledger_account_balance_t>{partial_balances,n_balances*ACCOUNT_SIZE/WORD_SIZE}));
            dump(mt->get_proof(balance_index * ACCOUNT_SIZE, log2_leaf_size), balance, drive_offset);
        } else if (dump_full_proof) {
            std::string config_path = machine_snapshot_path + "/config.json";
            if (!std::filesystem::exists(machine_snapshot_path) || !std::filesystem::exists(config_path)) {
                throw std::runtime_error{std::format("coundn'l find snapshot config '{}' ",config_path)};
            }

            std::ifstream config_file(config_path);
            if (!config_file.is_open()) {
                throw std::runtime_error{std::format("unable to open config '{}'",config_path)};
            }
            std::string config_json;
            std::string line;
            while (std::getline(config_file, line)) {
                config_json += line;
            }

            cm_machine *machine = nullptr;
            // if (cm_load_new(machine_snapshot_path.c_str(), config_json.c_str(), CM_SHARING_NONE, &machine) != CM_ERROR_OK) { // 0.20.0
            if (cm_load_new(machine_snapshot_path.c_str(), config_json.c_str(), &machine) != CM_ERROR_OK) {
              throw std::runtime_error{std::format("failed to load machine: {}", cm_get_last_error_message())};
            }
            const char *proof;
            size_t log2_account_size = static_cast<size_t>(std::log2(static_cast<double>(ACCOUNT_SIZE)));

            if (cm_get_proof(machine, flash_drive_offset + drive_offset, log2_account_size, &proof) != CM_ERROR_OK) {
              throw std::runtime_error{std::format("couldn't get proof: {}", cm_get_last_error_message())};
            }
            std::cout << proof << '\n';
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

        size_t log2_max_accounts = (n_balances < 1) ? 0 : std::bit_width(n_balances - 1);
        size_t log2_account_size = static_cast<size_t>(std::log2(static_cast<double>(ACCOUNT_SIZE)));

        auto index = account_balance_info.index;
        partial_ledger_account_balance_t *partial_balances = reinterpret_cast<partial_ledger_account_balance_t *>(balances);

        const auto log2_root_size = log2_max_accounts + log2_account_size;
        const auto log2_leaf_size = log2_account_size;
        const auto log2_word_size = LOG2_WORD_SIZE;
        std::unique_ptr<complete_merkle_tree> mt = std::make_unique<complete_merkle_tree>(
            log2_root_size, log2_word_size, log2_word_size,
            leaf_hashes(std::span<partial_ledger_account_balance_t>{partial_balances,n_balances*ACCOUNT_SIZE/WORD_SIZE}));
        dump(mt->get_proof(index * ACCOUNT_SIZE, log2_leaf_size), *(account_balance_info.balance), account_balance_info.offset);
    } else if (dump_full_proof) {
        std::string config_path = machine_snapshot_path + "/config.json";
        if (!std::filesystem::exists(machine_snapshot_path) || !std::filesystem::exists(config_path)) {
            throw std::runtime_error{std::format("coundn'l find snapshot config '{}' ",config_path)};
        }

        std::ifstream config_file(config_path);
        if (!config_file.is_open()) {
            throw std::runtime_error{std::format("unable to open config '{}'",config_path)};
        }
        std::string config_json;
        std::string line;
        while (std::getline(config_file, line)) {
            config_json += line;
        }

        cm_machine *machine = nullptr;
        // if (cm_load_new(machine_snapshot_path.c_str(), config_json.c_str(), CM_SHARING_NONE, &machine) != CM_ERROR_OK) { // 0.20.0
        if (cm_load_new(machine_snapshot_path.c_str(), config_json.c_str(), &machine) != CM_ERROR_OK) {
          throw std::runtime_error{std::format("failed to load machine: {}", cm_get_last_error_message())};
        }
        const char *proof;
        size_t log2_account_size = static_cast<size_t>(std::log2(static_cast<double>(ACCOUNT_SIZE)));

        if (cm_get_proof(machine, flash_drive_offset + account_balance_info.offset, log2_account_size, &proof) != CM_ERROR_OK) {
          throw std::runtime_error{std::format("couldn't get proof: {}", cm_get_last_error_message())};
        }
        std::cout << proof << '\n';
    } else {
        dump(account_balance_info);
    }

    return 0;
} catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
}
