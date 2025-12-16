
import logging
import os
from pycm import ffi, lib

ETHER_PORTAL_ADDRESS = [0xc7, 0x00, 0x76, 0xa4, 0x66, 0x78, 0x9B, 0x59, 0x5b, 0x50, 0x95, 0x9c, 0xdc, 0x26, 0x12, 0x27, 0xF0, 0xD7, 0x00, 0x51]
ERC20_PORTAL_ADDRESS = [0xc7, 0x00, 0xD6, 0xaD, 0xd0, 0x16, 0xeE, 0xCd, 0x59, 0xd9, 0x89, 0xC0, 0x28, 0x21, 0x4E, 0xaa, 0x0f, 0xCC, 0x00, 0x51]

def compare_lists(list1, list2):
    if len(list2) < len(list1):
        return False
    for i in range(len(list1)):
        if list1[i] != list2[i]:
            return False
    return True

logging.basicConfig(level="INFO")
logger = logging.getLogger(__name__)


rollup = ffi.new("cmt_rollup_t *")
assert(lib.cmt_rollup_init(rollup) == 0)
logger.info("[app] Initialized rollup device")

###
# Handlers

def handle_advance():
    logger.info("[app] Received advance request data")
    input = ffi.new("cmt_rollup_advance_t *")

    err = lib.cmt_rollup_read_advance_state(rollup, input)
    if err != 0:
        logger.error(f"[app] Failed to read advance: {err} {os.strerror(-err)}")
        exit(-1)

    # Ether Deposit?
    if compare_lists(input.msg_sender.data, ETHER_PORTAL_ADDRESS):
        logger.info("[app] Received ether deposit")

        # decode deposit
        parser_input = ffi.new("cma_parser_input_t *")
        err = lib.cma_parser_decode_advance(
            lib.CMA_PARSER_INPUT_TYPE_ETHER_DEPOSIT, input, parser_input)
        if err != 0:
            logger.error(f"[app] unable to decode ether deposit: {err} {lib.cma_parser_get_last_error_message()}")
            return True # False

        # encode voucher
        voucher_req = ffi.new("cma_parser_voucher_data_t *")
        voucher_req.receiver.data = parser_input.ether_deposit.sender.data
        voucher_req.ether.amount.data = parser_input.ether_deposit.amount.data

        voucher = ffi.new("cma_voucher_t *")
        addr = ffi.new("cma_abi_address_t *")

        err = lib.cma_parser_encode_voucher(lib.CMA_PARSER_VOUCHER_TYPE_ETHER, addr, voucher_req, voucher)
        if err != 0:
            logger.error(f"[app] unable to encode ether voucher: {err} {lib.cma_parser_get_last_error_message()}")
            return True # False

        # send voucher
        index   = ffi.new("uint64_t *")
        err = lib.cmt_rollup_emit_voucher(rollup,ffi.addressof(voucher,'address'),ffi.addressof(voucher,'value'),ffi.addressof(voucher,'payload'),index)
        if err != 0:
            logger.error(f"[app] unable to emit ether voucher: {err} {lib.cma_parser_get_last_error_message()}")
            return True # False

        logger.info(f"[app] emitted ether voucher: {index[0]=}")

        return True

    # Erc20 Deposit?
    if compare_lists(input.msg_sender.data, ERC20_PORTAL_ADDRESS):
        logger.info("[app] Received erc20 deposit")

        # decode deposit
        parser_input = ffi.new("cma_parser_input_t *")
        err = lib.cma_parser_decode_advance(
            lib.CMA_PARSER_INPUT_TYPE_ERC20_DEPOSIT, input, parser_input)
        if err != 0:
            logger.error(f"[app] unable to decode erc20 deposit: {err} {lib.cma_parser_get_last_error_message()}")
            return False

        # encode voucher
        voucher_req = ffi.new("cma_parser_voucher_data_t *")
        voucher_req.receiver.data = parser_input.erc20_deposit.sender.data
        voucher_req.erc20.token.data = parser_input.erc20_deposit.token.data
        voucher_req.erc20.amount.data = parser_input.erc20_deposit.amount.data

        voucher_payload_buf = ffi.new("uint8_t[CMA_PARSER_ERC20_VOUCHER_PAYLOAD_SIZE]")
        voucher = ffi.new("cma_voucher_t *")
        voucher.payload.data = voucher_payload_buf
        voucher.payload.length = len(voucher_payload_buf)
        addr = ffi.new("cma_abi_address_t *")
        addr.data = input.app_contract.data

        err = lib.cma_parser_encode_voucher(lib.CMA_PARSER_VOUCHER_TYPE_ERC20, addr, voucher_req, voucher)
        if err != 0:
            logger.error(f"[app] unable to encode erc20 voucher: {err} {lib.cma_parser_get_last_error_message()}")
            return False

        # send voucher
        index   = ffi.new("uint64_t *")
        err = lib.cmt_rollup_emit_voucher(rollup,ffi.addressof(voucher,'address'),ffi.addressof(voucher,'value'),ffi.addressof(voucher,'payload'),index)
        if err != 0:
            logger.error(f"[app] unable to emit erc20 voucher: {err} {lib.cma_parser_get_last_error_message()}")
            return False

        logger.info(f"[app] emitted erc20 voucher: {index[0]=}")

        return True

    logger.error("[app] invalid input")
    return True


def handle_inspect():
    logger.info("[app] Received inspect request data")
    input = ffi.new("cmt_rollup_inspect_t *")

    err = lib.cmt_rollup_read_inspect_state(rollup, input)
    if err != 0:
        logger.error(f"[app] Failed to read inspect: {err} {os.strerror(-err)}")
        return True

    logger.info(f"[app] inspect request with size {input.payload.length}")

    logger.error("[app] ignoring inspect request")
    return True


###
# Main loop


finish = ffi.new("cmt_rollup_finish_t *")
finish.accept_previous_request = True

while True:
    logger.info("[app] Sending finish")

    err = lib.cmt_rollup_finish(rollup, finish)
    if err != 0:
        logger.error(f"[app] Failed to send finish: {err} {os.strerror(-err)}")
        break

    logger.info(f"[app] Received finish status {finish.accept_previous_request}")

    if finish.next_request_type == lib.HTIF_YIELD_REASON_ADVANCE:
        finish.accept_previous_request = handle_advance()
    elif finish.next_request_type == lib.HTIF_YIELD_REASON_INSPECT:
        finish.accept_previous_request = handle_inspect()
    else:
        logger.error("[app] invalid request type")
        finish.accept_previous_request = False

        # finish["status"] = handler(rollup_request["data"])

exit(-1)
