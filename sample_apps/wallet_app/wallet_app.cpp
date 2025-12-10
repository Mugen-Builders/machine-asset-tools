#include <algorithm> // std::equal/std::copy_n
#include <array>     // std::ignore
#include <cerrno>    // errno
#include <cstdint>
#include <cstdio>  // std::fprintf/stderr
#include <cstring> // std::error
#include <string> // std::error
#include <tuple>   // std::ignore

extern "C" {
#include <sys/stat.h> // stat

#include <libcmt/abi.h>
#include <libcmt/io.h>
#include <libcmt/rollup.h>

#include <libcma/parser.h>
#include <libcma/ledger.h>
}

#define MERKLE_PATH "/tmp/merkle"

// 0xc70076a466789B595b50959cdc261227F0D70051
#define CONFIG_ETHER_PORTAL_ADDRESS {0xc7, 0x00, 0x76, 0xa4, 0x66, 0x78, 0x9B, 0x59, 0x5b, 0x50, 0x95, 0x9c, 0xdc, 0x26, 0x12, 0x27, 0xF0, 0xD7, 0x00, 0x51}

// 0xc700D6aDd016eECd59d989C028214Eaa0fCC0051
#define CONFIG_ERC20_PORTAL_ADDRESS {0xc7, 0x00, 0xD6, 0xaD, 0xd0, 0x16, 0xeE, 0xCd, 0x59, 0xd9, 0x89, 0xC0, 0x28, 0x21, 0x4E, 0xaa, 0x0f, 0xCC, 0x00, 0x51}

////////////////////////////////////////////////////////////////////////////////
// Abi utilities.

// Compare two ERC-20 addresses.
auto operator==(const cmt_abi_address_t &a, const cmt_abi_address_t &b) -> bool {
    return std::equal(std::begin(a.data), std::end(a.data), std::begin(b.data));
}

namespace {

class AppException : public std::exception {
private:
    std::string message;
    int errorCode;

public:
    // Constructor to initialize message and error code
    AppException(std::string msg, int code) : message(std::move(msg)), errorCode(code) {}

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

////////////////////////////////////////////////////////////////////////////////
// Rollup utilities.

template <typename T>
[[nodiscard]]
constexpr auto payload_to_bytes(const T &payload) -> cmt_abi_bytes_t {
    cmt_abi_bytes_t payload_bytes = {
        .length = sizeof(T),
        .data = const_cast<T *>(&payload) // NOLINT(cppcoreguidelines-pro-type-const-cast)
    };
    return payload_bytes;
}

// Emit a voucher POD into rollup device.
[[nodiscard]]
auto rollup_emit_voucher(cmt_rollup_t *rollup, const cmt_abi_address_t &address, const cmt_abi_u256_t &wei,
    const cmt_abi_bytes_t &payload) -> bool {
    const int err = cmt_rollup_emit_voucher(rollup, &address, &wei, &payload, nullptr);
    if (err < 0) {
        std::ignore = std::fprintf(stderr, "[app] unable to emit voucher: %s\n", std::strerror(-err));
        return false;
    }
    return true;
}

// Emit a report POD into rollup device.
template <typename T>
[[nodiscard]]
auto rollup_emit_report(cmt_rollup_t *rollup, const T &payload) -> bool {
    const cmt_abi_bytes_t payload_bytes = payload_to_bytes(payload);
    const int err = cmt_rollup_emit_report(rollup, &payload_bytes);
    if (err < 0) {
        std::ignore = std::fprintf(stderr, "[app] unable to emit report: %s\n", std::strerror(-err));
        return false;
    }
    return true;
}

// Finish last rollup request, wait for next rollup request and process it.
// For every new request, reads an input POD and call backs its respective
// advance or inspect state handler.
template <typename ADVANCE_STATE, typename INSPECT_STATE>
[[nodiscard]]
auto rollup_process_next_request(cmt_rollup_t *rollup, cma_ledger_t *ledger, ADVANCE_STATE advance_state, INSPECT_STATE inspect_state,
    bool last_request_status) -> bool {

    // Finish previous request and wait for the next request.
    std::ignore = std::fprintf(stdout, "[app] finishing previous request with status %d\n", last_request_status);
    cmt_rollup_finish_t finish{.accept_previous_request = last_request_status};
    const int err = cmt_rollup_finish(rollup, &finish);
    if (err < 0) {
        std::ignore = std::fprintf(stderr, "[app] unable to perform rollup finish: %s\n", std::strerror(-err));
        return false;
    }

    // Handle request
    switch (finish.next_request_type) {
        case HTIF_YIELD_REASON_ADVANCE: { // Advance state.
            return advance_state(rollup, ledger);
        }
        case HTIF_YIELD_REASON_INSPECT: { // Inspect state.
            // Call inspect state handler.
            return inspect_state(rollup, ledger);
        }
        default: { // Invalid request.
            std::ignore = std::fprintf(stderr, "[app] invalid request type\n");
            return false;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// application.

constexpr cmt_abi_address_t ETHER_PORTAL_ADDRESS = {CONFIG_ETHER_PORTAL_ADDRESS};
constexpr cmt_abi_address_t ERC20_PORTAL_ADDRESS = {CONFIG_ERC20_PORTAL_ADDRESS};

// reports.
struct [[gnu::packed]] error_report {
    int status{};
};

auto get_ether_id_storage() -> cma_ledger_asset_id_t & {
    static thread_local cma_ledger_asset_id_t ether_id;
    return ether_id;
}

auto process_get_balance(cmt_rollup_t *rollup, cma_ledger_t *ledger, cma_account_id_t *account, cma_token_address_t *token, cma_token_id_t *token_id) -> void {
    int err;
    // get asset
    cma_ledger_asset_id_t lass_id;

    if (account == nullptr) {
        throw AppException("invalid account id ptr", -EINVAL);
    }

    if (token == nullptr) {
        lass_id = get_ether_id_storage();
    } else if (token_id == nullptr) {
        err = cma_ledger_retrieve_asset(ledger, &lass_id, token, nullptr,
                   CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS, CMA_LEDGER_OP_FIND);
        if (err != CMA_LEDGER_SUCCESS) {
            throw AppException(std::string("unable to retrieve asset: ")
                .append(cma_ledger_get_last_error_message()).c_str(), err);
        }
    } else {
        err = cma_ledger_retrieve_asset(ledger, &lass_id, token, token_id,
                   CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID, CMA_LEDGER_OP_FIND);
        if (err != CMA_LEDGER_SUCCESS) {
            throw AppException(std::string("unable to retrieve asset: ")
                .append(cma_ledger_get_last_error_message()).c_str(), err);
        }
    }

    // get account
    cma_ledger_account_id_t lacc_id;
    err = cma_ledger_retrieve_account(ledger, &lacc_id, nullptr, account, CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID, CMA_LEDGER_OP_FIND);
    if (err != CMA_LEDGER_SUCCESS) {
        throw AppException(std::string("unable to retrieve account: ")
            .append(cma_ledger_get_last_error_message()).c_str(), err);
    }

    // get token balance
    cma_amount_t out_balance = {};
    err = cma_ledger_get_balance(ledger, lass_id, lacc_id, &out_balance);
    if (err != CMA_LEDGER_SUCCESS) {
        throw AppException(std::string("unable to retrieve token balance: ")
            .append(cma_ledger_get_last_error_message()).c_str(), err);
    }

    // emit report
    std::ignore = rollup_emit_report(rollup, out_balance.data);
}

auto process_get_total_supply(cmt_rollup_t *rollup, cma_ledger_t *ledger, cma_token_address_t *token, cma_token_id_t *token_id) -> void {
    int err;

    // get asset
    cma_ledger_asset_id_t lass_id;

    if (token == nullptr) {
        lass_id = get_ether_id_storage();
    } else if (token_id == nullptr) {
        err = cma_ledger_retrieve_asset(ledger, &lass_id, token, nullptr,
                   CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS, CMA_LEDGER_OP_FIND);
        if (err != CMA_LEDGER_SUCCESS) {
            throw AppException(std::string("unable to retrieve asset: ")
                .append(cma_ledger_get_last_error_message()).c_str(), err);
        }
    } else {
        err = cma_ledger_retrieve_asset(ledger, &lass_id, token, token_id,
                   CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID, CMA_LEDGER_OP_FIND);
        if (err != CMA_LEDGER_SUCCESS) {
            throw AppException(std::string("unable to retrieve asset: ")
                .append(cma_ledger_get_last_error_message()).c_str(), err);
        }
    }

    // get token supply
    cma_amount_t out_supply = {};
    err = cma_ledger_get_total_supply(ledger, lass_id, &out_supply);
    if (err != CMA_LEDGER_SUCCESS) {
        throw AppException(std::string("unable to retrieve token supply: ")
            .append(cma_ledger_get_last_error_message()).c_str(), err);
    }

    // emit report
    std::ignore = rollup_emit_report(rollup, out_supply.data);
}

auto process_inspect(cmt_rollup_t *rollup, cma_ledger_t *ledger, cmt_rollup_inspect_t *input) -> void {
    // Decode operation
    cma_parser_input_t parser_input;
    int err = cma_parser_decode_inspect(CMA_PARSER_INPUT_TYPE_AUTO, input, &parser_input);
    if (err != CMA_PARSER_SUCCESS) {
        throw AppException(std::string("unable to decode input: ")
            .append(cma_parser_get_last_error_message()).c_str(), err);
    }

    std::ignore = std::fprintf(stdout,"[app] Input is type %d\n", parser_input.type);

    switch (parser_input.type) {
        case CMA_PARSER_INPUT_TYPE_BALANCE_ACCOUNT:
            process_get_balance(rollup, ledger, &parser_input.balance.account, nullptr, nullptr);
            break;
        case CMA_PARSER_INPUT_TYPE_BALANCE_ACCOUNT_TOKEN_ADDRESS:
            process_get_balance(rollup, ledger, &parser_input.balance.account, &parser_input.balance.token, nullptr);
            break;
        case CMA_PARSER_INPUT_TYPE_BALANCE_ACCOUNT_TOKEN_ADDRESS_ID:
            process_get_balance(rollup, ledger, &parser_input.balance.account, &parser_input.balance.token, &parser_input.balance.token_id);
            break;
        case CMA_PARSER_INPUT_TYPE_SUPPLY:
            process_get_total_supply(rollup, ledger, nullptr, nullptr);
            break;
        case CMA_PARSER_INPUT_TYPE_SUPPLY_TOKEN_ADDRESS:
            process_get_total_supply(rollup, ledger, &parser_input.supply.token, nullptr);
            break;
        case CMA_PARSER_INPUT_TYPE_SUPPLY_TOKEN_ADDRESS_ID:
            process_get_total_supply(rollup, ledger, &parser_input.supply.token, &parser_input.supply.token_id);
            break;
        default:
            throw AppException("Invalid parsed inspect decode type", -EINVAL);
    }
    // throw AppException("Invalid inspect decode type", -EINVAL);
}


// Ignore inspect state queries
auto inspect_state(cmt_rollup_t *rollup, cma_ledger_t *ledger) -> bool try {
    cmt_rollup_inspect_t input{};
    const int err = cmt_rollup_read_inspect_state(rollup, &input);
    if (err < 0) {
        std::ignore = std::fprintf(stderr, "[app] unable to read inspect state: %s\n", std::strerror(-err));
        return false;
    }
    std::ignore = std::fprintf(stdout, "[app] inspect request with size %zu\n ", input.payload.length);

    process_inspect(rollup, ledger, &input);

    return true;
} catch (const AppException &e) {
  std::ignore =
      std::fprintf(stderr, "[app] app exception caught: (%d) %s\n",
                   e.code(), e.what());
    std::ignore = rollup_emit_report(rollup, error_report{e.code()});
    return false;
} catch (const std::exception &e) {
    std::ignore =
        std::fprintf(stderr, "[app] exception caught: %s\n", e.what());
    std::ignore = rollup_emit_report(rollup, error_report{-EPERM});
    return false;
} catch (...) {
    std::ignore =
        std::fprintf(stderr, "[app] unknown exception caught\n");
    std::ignore = rollup_emit_report(rollup, error_report{-EPERM});
    return false;
}

auto process_ether_deposit(cma_ledger_t *ledger, cmt_rollup_advance_t *input) -> void {
    std::ignore = std::fprintf(stdout,"[app] received ether deposit\n");
    // decode deposit
    cma_parser_input_t parser_input;
    int err = cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_DEPOSIT, input, &parser_input);
    if (err < 0) {
        throw AppException(std::string("unable to decode ether deposit: ")
            .append(cma_parser_get_last_error_message()).c_str(), err);
    }

    // get account
    cma_ledger_account_t account_id;
    cma_ledger_account_id_t lacc_id;
    err = cma_ledger_retrieve_account(ledger, &lacc_id, &account_id, &parser_input.ether_deposit.sender.data,
        CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS, CMA_LEDGER_OP_FIND_OR_CREATE);
    if (err != CMA_LEDGER_SUCCESS) {
        throw AppException(std::string("unable to retrieve account: ")
            .append(cma_ledger_get_last_error_message()).c_str(), err);
    }

    // deposit
    err = cma_ledger_deposit(ledger, get_ether_id_storage(), lacc_id, &parser_input.ether_deposit.amount);
    if (err != CMA_LEDGER_SUCCESS) {
        throw AppException(std::string("unable to deposit ether: ")
            .append(cma_ledger_get_last_error_message()).c_str(), err);
    }

    std::ignore = std::fprintf(stdout,"[app] Received 0x");
    for (size_t i = 0; i < sizeof(parser_input.ether_deposit.amount.data); i++) {
        std::ignore = std::fprintf(stdout,"%02x", parser_input.ether_deposit.amount.data[i]);
    }
    std::ignore = std::fprintf(stdout," ether deposit from: 0x");
    for (size_t i = 0; i < sizeof(account_id.address.data); i++) {
        std::ignore = std::fprintf(stdout,"%02x", account_id.address.data[i]);
    }
    std::ignore = std::fprintf(stdout,"\n");
}

auto process_erc20_deposit(cma_ledger_t *ledger, cmt_rollup_advance_t *input) -> void {
    std::ignore = std::fprintf(stdout,"[app] received erc20 deposit\n");
    // decode deposit
    cma_parser_input_t parser_input;
    int err = cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_DEPOSIT, input, &parser_input);
    if (err < 0) {
        throw AppException(std::string("unable to decode erc20 deposit: ")
            .append(cma_parser_get_last_error_message()).c_str(), err);
    }

    // get asset
    cma_ledger_asset_id_t lass_id;
    err = cma_ledger_retrieve_asset(ledger, &lass_id, &parser_input.erc20_deposit.token, nullptr,
               CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS, CMA_LEDGER_OP_FIND_OR_CREATE);
    if (err != CMA_LEDGER_SUCCESS) {
        throw AppException(std::string("unable to retrieve asset: ")
            .append(cma_ledger_get_last_error_message()).c_str(), err);
    }

    // get account
    cma_ledger_account_t account_id;
    cma_ledger_account_id_t lacc_id;
    err = cma_ledger_retrieve_account(ledger, &lacc_id, &account_id, &parser_input.erc20_deposit.sender.data,
        CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS, CMA_LEDGER_OP_FIND_OR_CREATE);
    if (err != CMA_LEDGER_SUCCESS) {
        throw AppException(std::string("unable to retrieve account: ")
            .append(cma_ledger_get_last_error_message()).c_str(), err);
    }

    // deposit
    err = cma_ledger_deposit(ledger, lass_id, lacc_id, &parser_input.erc20_deposit.amount);
    if (err != CMA_LEDGER_SUCCESS) {
        throw AppException(std::string("unable to deposit erc20: ")
            .append(cma_ledger_get_last_error_message()).c_str(), err);
    }

    std::ignore = std::fprintf(stdout,"[app] Received 0x");
    for (size_t i = 0; i < sizeof(parser_input.erc20_deposit.amount.data); i++) {
        std::ignore = std::fprintf(stdout,"%02x", parser_input.erc20_deposit.amount.data[i]);
    }
    std::ignore = std::fprintf(stdout," of 0x");
    for (size_t i = 0; i < sizeof(parser_input.erc20_deposit.token.data); i++) {
        std::ignore = std::fprintf(stdout,"%02x", parser_input.erc20_deposit.token.data[i]);
    }
    std::ignore = std::fprintf(stdout," erc20 token deposit from: 0x");
    for (size_t i = 0; i < sizeof(account_id.address.data); i++) {
        std::ignore = std::fprintf(stdout,"%02x", account_id.address.data[i]);
    }
    std::ignore = std::fprintf(stdout,"\n");
}

auto process_ether_withdrawal_and_send_voucher(cmt_rollup_t *rollup, cma_ledger_t *ledger, cma_ledger_account_id_t lacc_id,
        cma_ledger_account_t *account_id, cma_parser_input_t *parser_input) -> void {

    // withdraw from ledger
    int err = cma_ledger_withdraw(ledger, get_ether_id_storage(), lacc_id, &parser_input->ether_withdrawal.amount);
    if (err != CMA_LEDGER_SUCCESS) {
        throw AppException(std::string("unable to withdraw ether: ")
            .append(cma_ledger_get_last_error_message()).c_str(), err);
    }

    // encode voucher
    cma_parser_voucher_data_t voucher_req = {};
    std::ignore =
        std::copy_n(std::begin(account_id->address.data), CMA_ABI_ADDRESS_LENGTH, std::begin(voucher_req.receiver.data));
    std::ignore = std::copy_n(std::begin(parser_input->ether_withdrawal.amount.data), CMA_ABI_U256_LENGTH,
        std::begin(voucher_req.ether.amount.data));

    cma_voucher_t voucher = {};

    err = cma_parser_encode_voucher(CMA_PARSER_VOUCHER_TYPE_ETHER, nullptr, &voucher_req, &voucher);
    if (err < 0) {
        throw AppException(std::string("unable to encode ether voucher: ")
            .append(cma_parser_get_last_error_message()).c_str(), err);
    }

    // send voucher
    if (!rollup_emit_voucher(rollup, voucher.address, voucher.value, voucher.payload)) {
        throw AppException(std::string("unable to emit ether voucher: ")
            .append(cma_parser_get_last_error_message()).c_str(), err);
    }
    std::ignore = std::fprintf(stdout,"[app] ether voucher sent\n");
}

auto process_erc20_withdrawal_and_send_voucher(cmt_rollup_t *rollup, cma_ledger_t *ledger, cma_ledger_account_id_t lacc_id,
        cma_ledger_account_t *account_id, cma_parser_input_t *parser_input) -> void {

    // get asset
    cma_ledger_asset_id_t lass_id;
    int err = cma_ledger_retrieve_asset(ledger, &lass_id, &parser_input->erc20_withdrawal.token, nullptr,
               CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS, CMA_LEDGER_OP_FIND_OR_CREATE);
    if (err != CMA_LEDGER_SUCCESS) {
        throw AppException(std::string("unable to retrieve asset: ")
            .append(cma_ledger_get_last_error_message()).c_str(), err);
    }

    // withdraw from ledger
    err = cma_ledger_withdraw(ledger, lass_id, lacc_id, &parser_input->erc20_withdrawal.amount);
    if (err != CMA_LEDGER_SUCCESS) {
        throw AppException(std::string("unable to withdraw erc20: ")
            .append(cma_ledger_get_last_error_message()).c_str(), err);
    }

    // encode voucher
    cma_parser_voucher_data_t voucher_req = {};
    std::ignore =
        std::copy_n(std::begin(account_id->address.data), CMA_ABI_ADDRESS_LENGTH, std::begin(voucher_req.receiver.data));
    std::ignore = std::copy_n(std::begin(parser_input->erc20_withdrawal.token.data), CMA_ABI_ADDRESS_LENGTH,
        std::begin(voucher_req.erc20.token.data));
    std::ignore = std::copy_n(std::begin(parser_input->erc20_withdrawal.amount.data), CMA_ABI_U256_LENGTH,
        std::begin(voucher_req.erc20.amount.data));

    uint8_t erc20_payload[CMA_PARSER_ERC20_VOUCHER_PAYLOAD_SIZE];
    cma_voucher_t voucher = {.payload = {.length = sizeof(erc20_payload), .data = erc20_payload}};

    err = cma_parser_encode_voucher(CMA_PARSER_VOUCHER_TYPE_ERC20, nullptr, &voucher_req, &voucher);
    if (err < 0) {
        throw AppException(std::string("unable to encode erc20 voucher: ")
            .append(cma_parser_get_last_error_message()).c_str(), err);
    }

    // send voucher
    if (!rollup_emit_voucher(rollup, voucher.address, voucher.value, voucher.payload)) {
        throw AppException(std::string("unable to emit erc20 voucher: ")
            .append(cma_parser_get_last_error_message()).c_str(), err);
    }
    std::ignore = std::fprintf(stdout,"[app] erc20 voucher sent\n");
}

auto process_ether_transfer(cma_ledger_t *ledger, cma_ledger_account_id_t lacc_id, cma_parser_input_t *parser_input) -> void {
    // get receiver account
    cma_ledger_account_id_t receiver_lacc_id;
    // cma_ledger_account_t receiver_account_id;
    int err = cma_ledger_retrieve_account(ledger, &receiver_lacc_id, nullptr, &parser_input->ether_transfer.receiver.data,
        CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID, CMA_LEDGER_OP_FIND_OR_CREATE);
    if (err != CMA_LEDGER_SUCCESS) {
        throw AppException(std::string("unable to retrieve receiver account: ")
            .append(cma_ledger_get_last_error_message()).c_str(), err);
    }

    // transfer from ledger
    err = cma_ledger_transfer(ledger, get_ether_id_storage(), lacc_id, receiver_lacc_id, &parser_input->ether_transfer.amount);
    if (err != CMA_LEDGER_SUCCESS) {
        throw AppException(std::string("unable to transfer ether: ")
            .append(cma_ledger_get_last_error_message()).c_str(), err);
    }

    std::ignore = std::fprintf(stdout,"[app] ether transfered\n");
}

auto process_erc20_transfer(cma_ledger_t *ledger, cma_ledger_account_id_t lacc_id, cma_parser_input_t *parser_input) -> void {
    // get receiver account
    cma_ledger_account_id_t receiver_lacc_id;
    int err = cma_ledger_retrieve_account(ledger, &receiver_lacc_id, nullptr, &parser_input->erc20_transfer.receiver.data,
        CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS, CMA_LEDGER_OP_FIND_OR_CREATE);
    if (err != CMA_LEDGER_SUCCESS) {
        throw AppException(std::string("unable to retrieve receiver account: ")
            .append(cma_ledger_get_last_error_message()).c_str(), err);
    }

    // get asset
    cma_ledger_asset_id_t lass_id;
    err = cma_ledger_retrieve_asset(ledger, &lass_id, &parser_input->erc20_transfer.token, nullptr,
               CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS, CMA_LEDGER_OP_FIND_OR_CREATE);
    if (err != CMA_LEDGER_SUCCESS) {
        throw AppException(std::string("unable to retrieve asset: ")
            .append(cma_ledger_get_last_error_message()).c_str(), err);
    }

    // transfer from ledger
    err = cma_ledger_transfer(ledger, lass_id, lacc_id, receiver_lacc_id, &parser_input->erc20_transfer.amount);
    if (err != CMA_LEDGER_SUCCESS) {
        throw AppException(std::string("unable to transfer erc20: ")
            .append(cma_ledger_get_last_error_message()).c_str(), err);
    }

    std::ignore = std::fprintf(stdout,"[app] erc20 transfered\n");
}

auto process_advance(cmt_rollup_t *rollup, cma_ledger_t *ledger, cmt_rollup_advance_t *input) -> void {
    // Decode operation
    cma_parser_input_t parser_input;
    int err = cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_AUTO, input, &parser_input);
    if (err != CMA_PARSER_SUCCESS) {
        throw AppException(std::string("unable to decode input: ")
            .append(cma_parser_get_last_error_message()).c_str(), err);
    }

    std::ignore = std::fprintf(stdout,"[app] Input is type %d\n", parser_input.type);

    // get account
    cma_ledger_account_id_t lacc_id;
    cma_ledger_account_t account_id;
    err = cma_ledger_retrieve_account(ledger, &lacc_id, &account_id, &input->msg_sender.data,
        CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS, CMA_LEDGER_OP_FIND_OR_CREATE);
    if (err != CMA_LEDGER_SUCCESS) {
        throw AppException(std::string("unable to retrieve account: ")
            .append(cma_ledger_get_last_error_message()).c_str(), err);
    }

    std::ignore = std::fprintf(stdout,"[app] sender account_id: 0x");
    for (size_t i = 0; i < sizeof(account_id.account_id.data); i++) {
        std::ignore = std::fprintf(stdout,"%02x", account_id.account_id.data[i]);
    }
    std::ignore = std::fprintf(stdout,"\n");

    switch (parser_input.type) {
        case CMA_PARSER_INPUT_TYPE_ETHER_WITHDRAWAL:
            process_ether_withdrawal_and_send_voucher(rollup, ledger, lacc_id, &account_id, &parser_input);
            break;
        case CMA_PARSER_INPUT_TYPE_ERC20_WITHDRAWAL:
            process_erc20_withdrawal_and_send_voucher(rollup, ledger, lacc_id, &account_id, &parser_input);
            break;
        case CMA_PARSER_INPUT_TYPE_ETHER_TRANSFER:
            process_ether_transfer(ledger, lacc_id, &parser_input);
            break;
        case CMA_PARSER_INPUT_TYPE_ERC20_TRANSFER:
            process_erc20_transfer(ledger, lacc_id, &parser_input);
            break;
        default:
            throw AppException("Invalid advance decode type", -EINVAL);
    }
    // throw AppException("invalid advance state request", -EINVAL);
}

// Process advance state requests
auto advance_state(cmt_rollup_t *rollup, cma_ledger_t *ledger) -> bool try {
    // Read the input.
    std::ignore = std::fprintf(stderr, "[app] advance request\n");
    cmt_rollup_advance_t input{};
    int err = cmt_rollup_read_advance_state(rollup, &input);
    if (err < 0) {
        // throw AppException(std::string("unable to read advance state: ")
        //     .append(std::strerror(-err)).c_str(), -EINVAL);
        std::ignore =
            std::fprintf(stderr, "[app] unable to read advance state: %s\n", std::strerror(-err));
        std::ignore = std::fprintf(stderr, "[app] forcing exit\n");
        exit(-EINVAL); // force exit. Exceptional error
    }

    std::ignore = std::fprintf(stdout, "[app] advance request with size %zu\n", input.payload.length);

    std::ignore = std::fprintf(stdout,"[app] msg_sender: 0x");
    for (size_t i = 0; i < sizeof(input.msg_sender.data); i++) {
        std::ignore = std::fprintf(stdout,"%02x", input.msg_sender.data[i]);
    }
    std::ignore = std::fprintf(stdout,"\n");

    // Ether Deposit?
    if (input.msg_sender == ETHER_PORTAL_ADDRESS) {
        process_ether_deposit(ledger, &input);
        return true;
    }

    // Erc20 Deposit?
    if (input.msg_sender == ERC20_PORTAL_ADDRESS) {
        process_erc20_deposit(ledger, &input);
        return true;
    }

    process_advance(rollup, ledger, &input);
    return true;
} catch (const AppException &e) {
  std::ignore =
      std::fprintf(stderr, "[app] app exception caught: (%d) %s\n",
                   e.code(), e.what());
    std::ignore = rollup_emit_report(rollup, error_report{e.code()});
    return true; // No reverts
} catch (const std::exception &e) {
    std::ignore =
        std::fprintf(stderr, "[app] exception caught: %s\n", e.what());
    std::ignore = rollup_emit_report(rollup, error_report{-EPERM});
    return true; // No reverts
} catch (...) {
    std::ignore =
        std::fprintf(stderr, "[app] unknown exception caught\n");
    std::ignore = rollup_emit_report(rollup, error_report{-EPERM});
    return true; // No reverts
}

}; // anonymous namespace

// Application main.
auto main() -> int {
    // Initialize ledger
    cma_ledger_t ledger;
    int err = cma_ledger_init(&ledger);
    if (err != CMA_LEDGER_SUCCESS) {
        std::ignore = std::fprintf(stderr, "[app] unable to Initialize ledger: (%d) %s\n", err, cma_ledger_get_last_error_message());
        return -1;
    }

    // create ether asset in ledger lib
    cma_ledger_asset_id_t asset_id;
    err = cma_ledger_retrieve_asset(&ledger, &asset_id, NULL, NULL, CMA_LEDGER_ASSET_TYPE_ID,
               CMA_LEDGER_OP_FIND_OR_CREATE);
    if (err != CMA_LEDGER_SUCCESS) {
        std::ignore = std::fprintf(stderr, "[app] unable to create ether asset: (%d) %s\n", err, cma_ledger_get_last_error_message());
        return -1;
    }
    get_ether_id_storage() = asset_id;

    cmt_rollup_t rollup{};
    // Disable buffering of stderr to avoid dynamic allocations behind the scenes
    if (std::setvbuf(stderr, nullptr, _IONBF, 0) != 0) {
        std::ignore = std::fprintf(stderr, "[app] unable to disable stderr buffering: %s\n", std::strerror(errno));
        return -1;
    }

    // Initialize rollup device.
    err = cmt_rollup_init(&rollup);
    if (err != 0) {
        std::ignore = std::fprintf(stderr, "[app] unable to initialize rollup device: %s\n", std::strerror(-err));
        return -1;
    }

    struct stat buffer;
    if (stat(MERKLE_PATH, &buffer) == 0) {
      const int err_merkle = cmt_rollup_load_merkle(&rollup, MERKLE_PATH);
      if (err_merkle != 0) {
        std::ignore = std::fprintf(stderr, "[app] unable to load merkle tree: %s\n", std::strerror(-err_merkle));
        return -1;
      }
    }

    // Process requests forever.
    std::ignore = std::fprintf(stderr, "[app] processing rollup requests...\n");
    bool accept_previous_request = true;
    while (true) {
        if (accept_previous_request) {
            mode_t original_umask = umask(0000);
            const int err_merkle = cmt_rollup_save_merkle(&rollup, MERKLE_PATH);
            if (err_merkle != 0) {
                std::ignore = std::fprintf(stderr, "[rives] unable to save merkle tree: %s\n", std::strerror(-err_merkle));
                return -1;
            }
            umask(original_umask);
        }
        // Always continue, despite request failing or not.
        accept_previous_request =
            rollup_process_next_request(&rollup, &ledger, advance_state, inspect_state, accept_previous_request);
    }
    // Unreachable code, return is intentionally omitted.
}
