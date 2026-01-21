#include <algorithm> // std::equal/std::copy_n
#include <array>     // std::ignore
#include <cerrno>    // errno
#include <cstdint>
#include <cstdio>  // std::fprintf/stderr
#include <cstring> // std::error
#include <tuple>   // std::ignore

extern "C" {
#include <libcmt/abi.h>
#include <libcmt/io.h>
#include <libcmt/rollup.h>

#include <libcma/parser.h>
}

#define CONFIG_ETHER_PORTAL_ADDRESS {0xa6, 0x32, 0xc5, 0xc0, 0x58, 0x12, 0xc6, 0xa6, 0x14, 0x9b, 0x7a, 0xf5, 0xc5, 0x61, 0x17, 0xd1, 0xd2, 0x60, 0x38, 0x28}
#define CONFIG_ERC20_PORTAL_ADDRESS {0xac, 0xa6, 0x58, 0x6a, 0xc, 0xf0, 0x5b, 0xd8, 0x31, 0xf2, 0x50, 0x1e, 0x7b, 0x4a, 0xea, 0x55, 0xd, 0xa6, 0x56, 0x2d}
#define CONFIG_ERC721_PORTAL_ADDRESS {0x9e, 0x88, 0x51, 0xda, 0xdb, 0x2b, 0x77, 0x10, 0x39, 0x28, 0x51, 0x88, 0x46, 0xc4, 0x67, 0x8d, 0x48, 0xb5, 0xe3, 0x71}
#define CONFIG_ERC1155_SINGLE_PORTAL_ADDRESS {0x18, 0x55, 0x83, 0x98, 0xdd, 0x1a, 0x8c, 0xe2, 0x9, 0x56, 0x28, 0x7a, 0x4d, 0xa7, 0xb7, 0x6a, 0xe7, 0xa9, 0x66, 0x62}
#define CONFIG_ERC1155_BATCH_PORTAL_ADDRESS {0xe2, 0x46, 0xab, 0xb9, 0x74, 0xb3, 0x7, 0x49, 0xd, 0x9c, 0x69, 0x32, 0xf4, 0x8e, 0xbe, 0x79, 0xde, 0x72, 0x33, 0x8a}

////////////////////////////////////////////////////////////////////////////////
// Abi utilities.

// Compare two ERC-20 addresses.
auto operator==(const cmt_abi_address_t &a, const cmt_abi_address_t &b) -> bool {
    return std::equal(std::begin(a.data), std::end(a.data), std::begin(b.data));
}

namespace {
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
auto rollup_process_next_request(cmt_rollup_t *rollup, ADVANCE_STATE advance_state, INSPECT_STATE inspect_state,
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
            return advance_state(rollup);
        }
        case HTIF_YIELD_REASON_INSPECT: { // Inspect state.
            // Call inspect state handler.
            return inspect_state(rollup);
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
constexpr cmt_abi_address_t ERC721_PORTAL_ADDRESS = {CONFIG_ERC721_PORTAL_ADDRESS};
constexpr cmt_abi_address_t ERC1155_SINGLE_PORTAL_ADDRESS = {CONFIG_ERC1155_SINGLE_PORTAL_ADDRESS};
constexpr cmt_abi_address_t ERC1155_BATCH_PORTAL_ADDRESS = {CONFIG_ERC1155_BATCH_PORTAL_ADDRESS};

// reports.
struct [[gnu::packed]] error_report {
    int status{};
};

// Ignore inspect state queries
auto inspect_state(cmt_rollup_t *rollup) -> bool {
    cmt_rollup_inspect_t input{};
    const int err = cmt_rollup_read_inspect_state(rollup, &input);
    if (err < 0) {
        std::ignore = std::fprintf(stderr, "[app] unable to read inspect state: %s\n", std::strerror(-err));
        return false;
    }
    std::ignore = std::fprintf(stdout, "[app] inspect request with size %zu\n", input.payload.length);

    std::ignore = std::fprintf(stderr, "[app] inspect ignored\n");
    return true;
}

// Process advance state requests
auto advance_state(cmt_rollup_t *rollup) -> bool {
    // Read the input.
    std::ignore = std::fprintf(stderr, "[app] advance request\n");
    cmt_rollup_advance_t input{};
    int err = cmt_rollup_read_advance_state(rollup, &input);
    if (err < 0) {
        std::ignore =
            std::fprintf(stderr, "[app] unable to read advance state: %s\n", std::strerror(-err));
        std::ignore = std::fprintf(stderr, "[app] forcing exit\n");
        exit(-EINVAL); // force exit. Exceptional error
    }

    std::ignore = std::fprintf(stdout, "[app] advance request with size %zu\n", input.payload.length);

    // Ether Deposit?
    if (input.msg_sender == ETHER_PORTAL_ADDRESS) {
        std::ignore = std::fprintf(stdout,"[app] received ether deposit\n");
        // decode deposit
        cma_parser_input_t parser_input;
        err = cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ETHER_DEPOSIT, &input, &parser_input);
        if (err < 0) {
            std::ignore = std::fprintf(stderr, "[app] unable to decode ether deposit: %d - %s\n", -err,
                cma_parser_get_last_error_message());
            std::ignore = rollup_emit_report(rollup, error_report{err});
            return false;
        }

        // encode voucher
        cma_parser_voucher_data_t voucher_req = {};
        std::ignore =
            std::copy_n(std::begin(parser_input.ether_deposit.sender.data), CMA_ABI_ADDRESS_LENGTH, std::begin(voucher_req.receiver.data));
        std::ignore = std::copy_n(std::begin(parser_input.ether_deposit.amount.data), CMA_ABI_U256_LENGTH,
            std::begin(voucher_req.ether.amount.data));

        cma_voucher_t voucher = {};

        err = cma_parser_encode_voucher(CMA_PARSER_VOUCHER_TYPE_ETHER, nullptr, &voucher_req, &voucher);
        if (err < 0) {
            std::ignore = std::fprintf(stderr, "[app] unable to encode ether voucher: %d - %s\n", -err,
                cma_parser_get_last_error_message());
            std::ignore = rollup_emit_report(rollup, error_report{err});
            return false;
        }

        // send voucher
        if (!rollup_emit_voucher(rollup, voucher.address, voucher.value, voucher.payload)) {
            std::ignore = std::fprintf(stderr, "[app] unable to emit ether voucher: %d - %s\n", -err,
                cma_parser_get_last_error_message());
            std::ignore = rollup_emit_report(rollup, error_report{-EINVAL});
            return false;
        }
        std::ignore = std::fprintf(stdout,"[app] voucher sent\n");
        return true;
    }

    // Erc20 Deposit?
    if (input.msg_sender == ERC20_PORTAL_ADDRESS) {
        std::ignore = std::fprintf(stdout,"[app] received erc20 deposit\n");
        // decode deposit
        cma_parser_input_t parser_input;
        err = cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC20_DEPOSIT, &input, &parser_input);
        if (err < 0) {
            std::ignore = std::fprintf(stderr, "[app] unable to decode erc20 deposit: %d - %s\n", -err,
                cma_parser_get_last_error_message());
            std::ignore = rollup_emit_report(rollup, error_report{err});
            return false;
        }

        // encode voucher
        cma_parser_voucher_data_t voucher_req = {};
        std::ignore =
            std::copy_n(std::begin(parser_input.erc20_deposit.sender.data), CMA_ABI_ADDRESS_LENGTH, std::begin(voucher_req.receiver.data));
        std::ignore = std::copy_n(std::begin(parser_input.erc20_deposit.token.data), CMA_ABI_ADDRESS_LENGTH,
            std::begin(voucher_req.erc20.token.data));
        std::ignore = std::copy_n(std::begin(parser_input.erc20_deposit.amount.data), CMA_ABI_U256_LENGTH,
            std::begin(voucher_req.erc20.amount.data));

        uint8_t erc20_payload[CMA_PARSER_ERC20_VOUCHER_PAYLOAD_SIZE];
        cma_voucher_t voucher = {.payload = {.length = sizeof(erc20_payload), .data = erc20_payload}};

        err = cma_parser_encode_voucher(CMA_PARSER_VOUCHER_TYPE_ERC20, nullptr, &voucher_req, &voucher);
        if (err < 0) {
            std::ignore = std::fprintf(stderr, "[app] unable to encode erc20 voucher: %d - %s\n", -err,
                cma_parser_get_last_error_message());
            std::ignore = rollup_emit_report(rollup, error_report{err});
            return false;
        }

        // send voucher
        if (!rollup_emit_voucher(rollup, voucher.address, voucher.value, voucher.payload)) {
            std::ignore = std::fprintf(stderr, "[app] unable to emit erc20 voucher: %d - %s\n", -err,
                cma_parser_get_last_error_message());
            std::ignore = rollup_emit_report(rollup, error_report{-EINVAL});
            return false;
        }
        std::ignore = std::fprintf(stdout,"[app] voucher sent\n");
        return true;
    }

    // Erc721 Deposit?
    if (input.msg_sender == ERC721_PORTAL_ADDRESS) {
        std::ignore = std::fprintf(stdout,"[app] received erc721 deposit\n");
        // decode deposit
        cma_parser_input_t parser_input;
        err = cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC721_DEPOSIT, &input, &parser_input);
        if (err < 0) {
            std::ignore = std::fprintf(stderr, "[app] unable to decode erc721 deposit: %d - %s\n", -err,
                cma_parser_get_last_error_message());
            std::ignore = rollup_emit_report(rollup, error_report{err});
            return false;
        }

        // encode voucher
        cma_parser_voucher_data_t voucher_req = {};
        std::ignore =
            std::copy_n(std::begin(parser_input.erc721_deposit.sender.data), CMA_ABI_ADDRESS_LENGTH, std::begin(voucher_req.receiver.data));
        std::ignore = std::copy_n(std::begin(parser_input.erc721_deposit.token.data), CMA_ABI_ADDRESS_LENGTH,
            std::begin(voucher_req.erc721.token.data));
        std::ignore = std::copy_n(std::begin(parser_input.erc721_deposit.token_id.data), CMA_ABI_U256_LENGTH,
            std::begin(voucher_req.erc721.token_id.data));

        uint8_t erc721_payload[CMA_PARSER_ERC721_VOUCHER_PAYLOAD_SIZE];
        cma_voucher_t voucher = {.payload = {.length = sizeof(erc721_payload), .data = erc721_payload}};

        err = cma_parser_encode_voucher(CMA_PARSER_VOUCHER_TYPE_ERC721, &input.app_contract, &voucher_req, &voucher);
        if (err < 0) {
            std::ignore = std::fprintf(stderr, "[app] unable to encode erc721 voucher: %d - %s\n", -err,
                cma_parser_get_last_error_message());
            std::ignore = rollup_emit_report(rollup, error_report{-EINVAL});
            return false;
        }

        // send voucher
        if (!rollup_emit_voucher(rollup, voucher.address, voucher.value, voucher.payload)) {
            std::ignore = std::fprintf(stderr, "[app] unable to emit erc721 voucher: %d - %s\n", -err,
                cma_parser_get_last_error_message());
            std::ignore = rollup_emit_report(rollup, error_report{-EINVAL});
            return false;
        }
        std::ignore = std::fprintf(stdout,"[app] erc721 voucher sent\n");
        return true;
    }

    // Erc1155 single Deposit?
    if (input.msg_sender == ERC1155_SINGLE_PORTAL_ADDRESS) {
        std::ignore = std::fprintf(stdout,"[app] received erc1155 single deposit\n");
        // decode deposit
        cma_parser_input_t parser_input;
        err = cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC1155_SINGLE_DEPOSIT, &input, &parser_input);
        if (err < 0) {
            std::ignore = std::fprintf(stderr, "[app] unable to decode erc1155 single deposit: %d - %s\n", -err,
                cma_parser_get_last_error_message());
            std::ignore = rollup_emit_report(rollup, error_report{err});
            return false;
        }

        // encode voucher
        cma_parser_voucher_data_t voucher_req = {};
        std::ignore =
            std::copy_n(std::begin(parser_input.erc1155_single_deposit.sender.data), CMA_ABI_ADDRESS_LENGTH, std::begin(voucher_req.receiver.data));
        std::ignore = std::copy_n(std::begin(parser_input.erc1155_single_deposit.token.data), CMA_ABI_ADDRESS_LENGTH,
            std::begin(voucher_req.erc1155_single.token.data));
        std::ignore = std::copy_n(std::begin(parser_input.erc1155_single_deposit.token_id.data), CMA_ABI_U256_LENGTH,
            std::begin(voucher_req.erc1155_single.token_id.data));
        std::ignore = std::copy_n(std::begin(parser_input.erc1155_single_deposit.amount.data), CMA_ABI_U256_LENGTH,
            std::begin(voucher_req.erc1155_single.amount.data));

        uint8_t erc1155_single_payload[CMA_PARSER_ERC1155_SINGLE_VOUCHER_PAYLOAD_MIN_SIZE];
        cma_voucher_t voucher = {.payload = {.length = sizeof(erc1155_single_payload), .data = erc1155_single_payload}};

        err = cma_parser_encode_voucher(CMA_PARSER_VOUCHER_TYPE_ERC1155_SINGLE, &input.app_contract, &voucher_req, &voucher);
        if (err < 0) {
            std::ignore = std::fprintf(stderr, "[app] unable to encode erc1155_single voucher: %d - %s\n", -err,
                cma_parser_get_last_error_message());
            std::ignore = rollup_emit_report(rollup, error_report{-EINVAL});
            return false;
        }

        // send voucher
        if (!rollup_emit_voucher(rollup, voucher.address, voucher.value, voucher.payload)) {
            std::ignore = std::fprintf(stderr, "[app] unable to emit erc1155_single voucher: %d - %s\n", -err,
                cma_parser_get_last_error_message());
            std::ignore = rollup_emit_report(rollup, error_report{-EINVAL});
            return false;
        }
        std::ignore = std::fprintf(stdout,"[app] erc1155_single voucher sent\n");
        return true;
    }

    // Erc1155 batch Deposit?
    if (input.msg_sender == ERC1155_BATCH_PORTAL_ADDRESS) {
        std::ignore = std::fprintf(stdout,"[app] received erc1155 batch deposit\n");
        // decode deposit
        cma_parser_input_t parser_input;
        err = cma_parser_decode_advance(CMA_PARSER_INPUT_TYPE_ERC1155_BATCH_DEPOSIT, &input, &parser_input);
        if (err < 0) {
            std::ignore = std::fprintf(stderr, "[app] unable to decode erc1155 batch deposit: %d - %s\n", -err,
                cma_parser_get_last_error_message());
            std::ignore = rollup_emit_report(rollup, error_report{err});
            return false;
        }

        // encode voucher
        cma_parser_voucher_data_t voucher_req = {};
        std::ignore =
            std::copy_n(std::begin(parser_input.erc1155_batch_deposit.sender.data), CMA_ABI_ADDRESS_LENGTH, std::begin(voucher_req.receiver.data));
        std::ignore = std::copy_n(std::begin(parser_input.erc1155_batch_deposit.token.data), CMA_ABI_ADDRESS_LENGTH,
            std::begin(voucher_req.erc1155_batch.token.data));
        voucher_req.erc1155_batch.token_ids.length = parser_input.erc1155_batch_deposit.token_ids.length;
        voucher_req.erc1155_batch.token_ids.data = parser_input.erc1155_batch_deposit.token_ids.data;
        voucher_req.erc1155_batch.amounts.length = parser_input.erc1155_batch_deposit.amounts.length;
        voucher_req.erc1155_batch.amounts.data = parser_input.erc1155_batch_deposit.amounts.data;

        size_t payload_size = CMA_PARSER_ERC1155_BATCH_VOUCHER_PAYLOAD_MIN_SIZE +
            2 * parser_input.erc1155_batch_deposit.token_ids.length * CMA_ABI_U256_LENGTH;
        uint8_t *erc1155_batch_payload = new uint8_t[payload_size];
        cma_voucher_t voucher = {.payload = {.length = payload_size, .data = erc1155_batch_payload}};

        err = cma_parser_encode_voucher(CMA_PARSER_VOUCHER_TYPE_ERC1155_BATCH, &input.app_contract, &voucher_req, &voucher);
        if (err < 0) {
            std::ignore = std::fprintf(stderr, "[app] unable to encode erc1155_batch voucher: %d - %s\n", -err,
                cma_parser_get_last_error_message());
            std::ignore = rollup_emit_report(rollup, error_report{-EINVAL});
            return false;
        }

        // send voucher
        if (!rollup_emit_voucher(rollup, voucher.address, voucher.value, voucher.payload)) {
            std::ignore = std::fprintf(stderr, "[app] unable to emit erc1155_batch voucher: %d - %s\n", -err,
                cma_parser_get_last_error_message());
            std::ignore = rollup_emit_report(rollup, error_report{-EINVAL});
            return false;
        }
        std::ignore = std::fprintf(stdout,"[app] erc1155_batch voucher sent\n");
        delete[] erc1155_batch_payload;
        erc1155_batch_payload = nullptr;
        return true;
    }

    // Invalid request.
    std::ignore = std::fprintf(stderr, "[app] invalid advance state request\n");
    std::ignore = rollup_emit_report(rollup, error_report{-EINVAL});
    return false;
}

}; // anonymous namespace

// Application main.
auto main() -> int {
    cmt_rollup_t rollup{};
    // Disable buffering of stderr to avoid dynamic allocations behind the scenes
    if (std::setvbuf(stderr, nullptr, _IONBF, 0) != 0) {
        std::ignore = std::fprintf(stderr, "[app] unable to disable stderr buffering: %s\n", std::strerror(errno));
        return -1;
    }

    // Initialize rollup device.
    const int err = cmt_rollup_init(&rollup);
    if (err != 0) {
        std::ignore = std::fprintf(stderr, "[app] unable to initialize rollup device: %s\n", std::strerror(-err));
        return -1;
    }
    // Process requests forever.
    std::ignore = std::fprintf(stderr, "[app] processing rollup requests...\n");
    bool accept_previous_request = true;
    while (true) {
        // Always continue, despite request failing or not.
        accept_previous_request =
            rollup_process_next_request(&rollup, advance_state, inspect_state, accept_previous_request);
    }
    // Unreachable code, return is intentionally omitted.
}
