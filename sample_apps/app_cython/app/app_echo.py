
import logging
import os
from cmpy import Rollup, decode_advance, decode_inspect, decode_ether_deposit, decode_erc20_deposit, \
    decode_erc721_deposit, decode_erc1155_single_deposit, decode_erc1155_batch_deposit

ETHER_PORTAL_ADDRESS = "0xA632c5c05812c6a6149B7af5C56117d1D2603828"[2:].lower()
ERC20_PORTAL_ADDRESS = "0xACA6586A0Cf05bD831f2501E7B4aea550dA6562D"[2:].lower()
ERC721_PORTAL_ADDRESS = "0x9E8851dadb2b77103928518846c4678d48b5e371"[2:].lower()
ERC1155_SINGLE_PORTAL_ADDRESS = "0x18558398Dd1a8cE20956287a4Da7B76aE7A96662"[2:].lower()
ERC1155_BATCH_PORTAL_ADDRESS = "0xe246Abb974B307490d9C6932F48EbE79de72338A"[2:].lower()

logging.basicConfig(level="INFO")
logger = logging.getLogger(__name__)

def handle_advance(rollup):
    advance = rollup.read_advance_state()
    msg_sender = advance['msg_sender'].hex().lower()
    logger.info(f"[app] Received advance request from {msg_sender=}")

    if msg_sender == ETHER_PORTAL_ADDRESS:
        deposit = decode_ether_deposit(advance)
        logger.info(f"[app] Ether deposit decoded {deposit}")

        rollup.emit_ether_voucher(deposit['sender'], deposit['amount'])
        logger.info("[app] Ether voucher emitted")
        return True

    if msg_sender == ERC20_PORTAL_ADDRESS:
        deposit = decode_erc20_deposit(advance)
        logger.info(f"[app] ERC20 deposit decoded {deposit}")

        rollup.emit_erc20_voucher(deposit['token'], deposit['sender'], deposit['amount'])
        logger.info("[app] Erc20 voucher emitted")
        return True

    if msg_sender == ERC721_PORTAL_ADDRESS:
        deposit = decode_erc721_deposit(advance)
        logger.info(f"[app] ERC721 deposit decoded {deposit}")

        rollup.emit_erc721_voucher(deposit['token'], deposit['sender'], deposit['token_id'])
        logger.info("[app] Erc721 voucher emitted")
        return True

    if msg_sender == ERC1155_SINGLE_PORTAL_ADDRESS:
        deposit = decode_erc1155_single_deposit(advance)
        logger.info(f"[app] ERC1155_single deposit decoded {deposit}")

        rollup.emit_erc1155_single_voucher(deposit['token'], deposit['sender'], deposit['token_id'], deposit['amount'])
        logger.info("[app] Erc1155_single voucher emitted")
        return True

    if msg_sender == ERC1155_BATCH_PORTAL_ADDRESS:
        logger.info(f"[app] ERC1155_batch deposit")
        deposit = decode_erc1155_batch_deposit(advance)
        logger.info(f"[app] ERC1155_batch deposit decoded {deposit}")

        rollup.emit_erc1155_batch_voucher(deposit['token'], deposit['sender'], deposit['token_ids'], deposit['amounts'])
        logger.info("[app] Erc1155_batch voucher emitted")
        return True

    try:
        decoded_advance = decode_advance(advance)
        logger.info(f"[app] Advance decoded {decoded_advance}")
        return True
    except Exception as e:
        logger.error(f"[app] Failed to decode advance: {e}")
        logger.info("[app] unidentified input")

    rollup.emit_report(advance['payload']['data'])
    return True

def handle_inspect(rollup):
    inspect = rollup.read_inspect_state()
    logger.info(f"Received inspect request length {len(inspect['payload']['data'])}")
    try:
        decoded_inspect = decode_inspect(inspect)
        logger.info(f"[app] Inspect decoded {decoded_inspect}")
        return True
    except Exception as e:
        logger.error(f"[app] Failed to decode inspect: {e}")
        logger.info("[app] unidentified input")

    rollup.emit_report(inspect['payload']['data'])
    return True

handlers = {
    "advance": handle_advance,
    "inspect": handle_inspect,
}

###
# Main
if __name__ == "__main__":
    rollup = Rollup()
    accept_previous_request = True

    # Main loop
    while True:
        logger.info("[app] Sending finish")

        next_request_type = rollup.finish(accept_previous_request)

        logger.info(f"[app] Received input of type {next_request_type}")

        accept_previous_request = handlers[next_request_type](rollup)

    exit(-1)
