
import logging
import os
from pycm import ffi, lib

ETHER_PORTAL_ADDRESS = [0xa6, 0x32, 0xc5, 0xc0, 0x58, 0x12, 0xc6, 0xa6, 0x14, 0x9b, 0x7a, 0xf5, 0xc5, 0x61, 0x17, 0xd1, 0xd2, 0x60, 0x38, 0x28]
ERC20_PORTAL_ADDRESS = [0xac, 0xa6, 0x58, 0x6a, 0xc, 0xf0, 0x5b, 0xd8, 0x31, 0xf2, 0x50, 0x1e, 0x7b, 0x4a, 0xea, 0x55, 0xd, 0xa6, 0x56, 0x2d]
ERC721_PORTAL_ADDRESS = [0x9e, 0x88, 0x51, 0xda, 0xdb, 0x2b, 0x77, 0x10, 0x39, 0x28, 0x51, 0x88, 0x46, 0xc4, 0x67, 0x8d, 0x48, 0xb5, 0xe3, 0x71]
ERC1155_SINGLE_PORTAL_ADDRESS = [0x18, 0x55, 0x83, 0x98, 0xdd, 0x1a, 0x8c, 0xe2, 0x9, 0x56, 0x28, 0x7a, 0x4d, 0xa7, 0xb7, 0x6a, 0xe7, 0xa9, 0x66, 0x62]
ERC1155_BATCH_PORTAL_ADDRESS = [0xe2, 0x46, 0xab, 0xb9, 0x74, 0xb3, 0x7, 0x49, 0xd, 0x9c, 0x69, 0x32, 0xf4, 0x8e, 0xbe, 0x79, 0xde, 0x72, 0x33, 0x8a]

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

    # Erc721 Deposit?
    if compare_lists(input.msg_sender.data, ERC721_PORTAL_ADDRESS):
        logger.info("[app] Received erc721 deposit")

        # decode deposit
        parser_input = ffi.new("cma_parser_input_t *")
        err = lib.cma_parser_decode_advance(
            lib.CMA_PARSER_INPUT_TYPE_ERC721_DEPOSIT, input, parser_input)
        if err != 0:
            logger.error(f"[app] unable to decode erc721 deposit: {err} {lib.cma_parser_get_last_error_message()}")
            return False

        # encode voucher
        voucher_req = ffi.new("cma_parser_voucher_data_t *")
        voucher_req.receiver.data = parser_input.erc721_deposit.sender.data
        voucher_req.erc721.token.data = parser_input.erc721_deposit.token.data
        voucher_req.erc721.token_id.data = parser_input.erc721_deposit.token_id.data

        voucher_payload_buf = ffi.new("uint8_t[CMA_PARSER_ERC721_VOUCHER_PAYLOAD_SIZE]")
        voucher = ffi.new("cma_voucher_t *")
        voucher.payload.data = voucher_payload_buf
        voucher.payload.length = len(voucher_payload_buf)
        addr = ffi.new("cma_abi_address_t *")
        addr.data = input.app_contract.data

        err = lib.cma_parser_encode_voucher(lib.CMA_PARSER_VOUCHER_TYPE_ERC721, addr, voucher_req, voucher)
        if err != 0:
            logger.error(f"[app] unable to encode erc721 voucher: {err} {lib.cma_parser_get_last_error_message()}")
            return False

        # send voucher
        index   = ffi.new("uint64_t *")
        err = lib.cmt_rollup_emit_voucher(rollup,ffi.addressof(voucher,'address'),ffi.addressof(voucher,'value'),ffi.addressof(voucher,'payload'),index)
        if err != 0:
            logger.error(f"[app] unable to emit erc721 voucher: {err} {lib.cma_parser_get_last_error_message()}")
            return False

        logger.info(f"[app] emitted erc721 voucher: {index[0]=}")

        return True

    # Erc1155 single Deposit?
    if compare_lists(input.msg_sender.data, ERC1155_SINGLE_PORTAL_ADDRESS):
        logger.info("[app] Received erc1155_single deposit")

        # decode deposit
        parser_input = ffi.new("cma_parser_input_t *")
        err = lib.cma_parser_decode_advance(
            lib.CMA_PARSER_INPUT_TYPE_ERC1155_SINGLE_DEPOSIT, input, parser_input)
        if err != 0:
            logger.error(f"[app] unable to decode erc1155_single deposit: {err} {lib.cma_parser_get_last_error_message()}")
            return False

        # encode voucher
        voucher_req = ffi.new("cma_parser_voucher_data_t *")
        voucher_req.receiver.data = parser_input.erc1155_single_deposit.sender.data
        voucher_req.erc1155_single.token.data = parser_input.erc1155_single_deposit.token.data
        voucher_req.erc1155_single.token_id.data = parser_input.erc1155_single_deposit.token_id.data
        voucher_req.erc1155_single.amount.data = parser_input.erc1155_single_deposit.amount.data

        voucher_payload_buf = ffi.new("uint8_t[CMA_PARSER_ERC1155_SINGLE_VOUCHER_PAYLOAD_MIN_SIZE]")
        voucher = ffi.new("cma_voucher_t *")
        voucher.payload.data = voucher_payload_buf
        voucher.payload.length = len(voucher_payload_buf)
        addr = ffi.new("cma_abi_address_t *")
        addr.data = input.app_contract.data

        err = lib.cma_parser_encode_voucher(lib.CMA_PARSER_VOUCHER_TYPE_ERC1155_SINGLE, addr, voucher_req, voucher)
        if err != 0:
            logger.error(f"[app] unable to encode erc1155_single voucher: {err} {lib.cma_parser_get_last_error_message()}")
            return False

        # send voucher
        index   = ffi.new("uint64_t *")
        err = lib.cmt_rollup_emit_voucher(rollup,ffi.addressof(voucher,'address'),ffi.addressof(voucher,'value'),ffi.addressof(voucher,'payload'),index)
        if err != 0:
            logger.error(f"[app] unable to emit erc1155_single voucher: {err} {lib.cma_parser_get_last_error_message()}")
            return False

        logger.info(f"[app] emitted erc1155_single voucher: {index[0]=}")

        return True

    # Erc1155 batch Deposit?
    if compare_lists(input.msg_sender.data, ERC1155_BATCH_PORTAL_ADDRESS):
        logger.info("[app] Received erc1155_batch deposit")

        # decode deposit
        parser_input = ffi.new("cma_parser_input_t *")
        err = lib.cma_parser_decode_advance(
            lib.CMA_PARSER_INPUT_TYPE_ERC1155_BATCH_DEPOSIT, input, parser_input)
        if err != 0:
            logger.error(f"[app] unable to decode erc1155_batch deposit: {err} {lib.cma_parser_get_last_error_message()}")
            return False

        # encode voucher
        voucher_req = ffi.new("cma_parser_voucher_data_t *")
        voucher_req.receiver.data = parser_input.erc1155_batch_deposit.sender.data
        voucher_req.erc1155_batch.token.data = parser_input.erc1155_batch_deposit.token.data
        voucher_req.erc1155_batch.token_ids.length = parser_input.erc1155_batch_deposit.token_ids.length
        voucher_req.erc1155_batch.token_ids.data = parser_input.erc1155_batch_deposit.token_ids.data
        voucher_req.erc1155_batch.amounts.length = parser_input.erc1155_batch_deposit.amounts.length
        voucher_req.erc1155_batch.amounts.data = parser_input.erc1155_batch_deposit.amounts.data

        voucher_payload_buf = ffi.new(f"uint8_t[{lib.CMA_PARSER_ERC1155_BATCH_VOUCHER_PAYLOAD_MIN_SIZE + 2 * parser_input.erc1155_batch_deposit.token_ids.length * lib.CMA_ABI_U256_LENGTH}]")
        voucher = ffi.new("cma_voucher_t *")
        voucher.payload.data = voucher_payload_buf
        voucher.payload.length = len(voucher_payload_buf)
        addr = ffi.new("cma_abi_address_t *")
        addr.data = input.app_contract.data

        err = lib.cma_parser_encode_voucher(lib.CMA_PARSER_VOUCHER_TYPE_ERC1155_BATCH, addr, voucher_req, voucher)
        if err != 0:
            logger.error(f"[app] unable to encode erc1155_batch voucher: {err} {lib.cma_parser_get_last_error_message()}")
            return False

        # send voucher
        index   = ffi.new("uint64_t *")
        err = lib.cmt_rollup_emit_voucher(rollup,ffi.addressof(voucher,'address'),ffi.addressof(voucher,'value'),ffi.addressof(voucher,'payload'),index)
        if err != 0:
            logger.error(f"[app] unable to emit erc1155_batch voucher: {err} {lib.cma_parser_get_last_error_message()}")
            return False

        logger.info(f"[app] emitted erc1155_batch voucher: {index[0]=}")

        return True

    logger.error("[app] invalid input")
    return False


def handle_inspect():
    logger.info("[app] Received inspect request data")
    input = ffi.new("cmt_rollup_inspect_t *")

    err = lib.cmt_rollup_read_inspect_state(rollup, input)
    if err != 0:
        logger.error(f"[app] Failed to read inspect: {err} {os.strerror(-err)}")
        return False

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
