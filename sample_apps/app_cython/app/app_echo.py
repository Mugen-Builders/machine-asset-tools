
import logging
import os
from cmpy import Rollup, decode_ether_deposit, decode_erc20_deposit, decode_advance, decode_inspect

ETHER_PORTAL_ADDRESS = '0xc70076a466789B595b50959cdc261227F0D70051'[2:].lower()
ERC20_PORTAL_ADDRESS = '0xc700D6aDd016eECd59d989C028214Eaa0fCC0051'[2:].lower()

logging.basicConfig(level="INFO")
logger = logging.getLogger(__name__)

def handle_advance(rollup):
    advance = rollup.read_advance_state()
    msg_sender = advance['msg_sender'].hex().lower()
    logger.info(f"Received advance request from {msg_sender=}")

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
