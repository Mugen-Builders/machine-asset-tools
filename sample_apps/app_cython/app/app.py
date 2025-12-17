
import logging
import os
from cmpy import Rollup, Ledger, decode_ether_deposit, decode_erc20_deposit, decode_advance, decode_inspect

ETHER_PORTAL_ADDRESS = '0xc70076a466789B595b50959cdc261227F0D70051'[2:].lower()
ERC20_PORTAL_ADDRESS = '0xc700D6aDd016eECd59d989C028214Eaa0fCC0051'[2:].lower()

logging.basicConfig(level="INFO")
logger = logging.getLogger(__name__)

class EtherId:
    ether_id = None
    def __new__(cls):
        return cls
    @classmethod
    def set(cls,val):
        cls.ether_id = val
    @classmethod
    def get(cls):
        return cls.ether_id

def handle_advance(rollup, ledger):
    advance = rollup.read_advance_state()
    msg_sender = advance['msg_sender'].hex().lower()
    logger.info(f"Received advance request from {msg_sender=}")

    if msg_sender == ETHER_PORTAL_ADDRESS:
        deposit = decode_ether_deposit(advance)

        account_info = ledger.retrieve_account(account=deposit['sender'])

        ledger.deposit(EtherId.get(), account_info['account_id'], deposit['amount'])
        logger.info(f"[app] {deposit['sender']} deposited {deposit['amount']} ether")
        return True

    if msg_sender == ERC20_PORTAL_ADDRESS:
        deposit = decode_erc20_deposit(advance)

        if not deposit['success']:
            logger.error("[app] ERC20 deposit failed")
            return False

        asset_info = ledger.retrieve_asset(token=deposit['token'])
        account_info = ledger.retrieve_account(account=deposit['sender'])

        ledger.deposit(asset_info['asset_id'], account_info['account_id'], deposit['amount'])
        logger.info(f"[app] {deposit['sender']} deposited {deposit['amount']} of token {asset_info['token']}")
        return True

    try:
        decoded_advance = decode_advance(advance)
        logger.info(f"[app] Advance is {decoded_advance['type']}")
        if decoded_advance['type'] == 'ETHER_WITHDRAWAL':
            account_info = ledger.retrieve_account(account=msg_sender)

            ledger.withdraw(EtherId.get(), account_info['account_id'], decoded_advance['amount'])
            logger.info(f"[app] {msg_sender} withdrew {decoded_advance['amount']} ethers")

            rollup.emit_ether_voucher(msg_sender, decoded_advance['amount'])
            logger.info("[app] Ether voucher emitted")
            return True

        if decoded_advance['type'] == 'ERC20_WITHDRAWAL':
            asset_info = ledger.retrieve_asset(token=decoded_advance['token'])
            account_info = ledger.retrieve_account(account=msg_sender)

            ledger.withdraw(asset_info['asset_id'], account_info['account_id'], decoded_advance['amount'])
            logger.info(f"[app] {msg_sender} withdrew {decoded_advance['amount']} of token {asset_info['token']}")

            rollup.emit_erc20_voucher(asset_info['token'], msg_sender, decoded_advance['amount'])
            logger.info("[app] Erc20 voucher emitted")
            return True

        if decoded_advance['type'] == 'ETHER_TRANSFER':
            account_info_from = ledger.retrieve_account(account=msg_sender)
            account_info_to = ledger.retrieve_account(account=decoded_advance['receiver'])

            ledger.transfer(EtherId.get(), account_info_from['account_id'], account_info_to['account_id'], decoded_advance['amount'])
            logger.info(f"[app] {msg_sender} transfered to {account_info_to['account']} {decoded_advance['amount']} ethers")
            return True

        if decoded_advance['type'] == 'ERC20_TRANSFER':
            asset_info = ledger.retrieve_asset(token=decoded_advance['token'])
            account_info_from = ledger.retrieve_account(account=msg_sender)
            account_info_to = ledger.retrieve_account(account=decoded_advance['receiver'])

            ledger.transfer(asset_info['asset_id'], account_info_from['account_id'], account_info_to['account_id'], decoded_advance['amount'])
            logger.info(f"[app] {msg_sender} transfered to {account_info_to['account']} {decoded_advance['amount']} of token {asset_info['token']}")
            return True

        logger.info("[app] unidentified wallet input")
        return False
    except Exception as e:
        logger.error(f"[app] Failed to process advance: {e}")

    logger.info("[app] non valid wallet input")
    return False

def handle_inspect(rollup, ledger):
    inspect = rollup.read_inspect_state()
    logger.info(f"Received inspect request length {len(inspect['payload']['data'])}")
    try:
        decoded_inspect = decode_inspect(inspect)
        logger.info(f"[app] Inspect decoded {decoded_inspect}")
        if decoded_inspect['type'] == 'BALANCE':
            account_info = ledger.retrieve_account(account=decoded_inspect['account'])
            asset_id = EtherId.get()
            if decoded_inspect['token'] is not None:
                asset_info = ledger.retrieve_asset(token=decoded_inspect['token'], token_id=decoded_inspect['token_id'])
                asset_id = asset_info['asset_id']

            current_balance = ledger.balance(asset_id, account_info['account_id'])
            logger.info(f"[app] {decoded_inspect['account']} balance is {current_balance}")

            rollup.emit_report(current_balance.to_bytes(32, 'big'))
            logger.info("[app] report emitted")
            return True
        if decoded_inspect['type'] == 'SUPPLY':
            asset_id = EtherId.get()
            if decoded_inspect['token'] is not None:
                asset_info = ledger.retrieve_asset(token=decoded_inspect['token'], token_id=decoded_inspect['token_id'])
                asset_id = asset_info['asset_id']

            current_supply = ledger.supply(asset_id)
            logger.info(f"[app] Asset supply is {current_supply}")

            rollup.emit_report(current_supply.to_bytes(32, 'big'))
            logger.info("[app] report emitted")
            return True

        logger.info("[app] unidentified wallet input")
        return False
    except Exception as e:
        logger.error(f"[app] Failed to process inspect: {e}")

    logger.info("[app] non valid wallet input")
    return False

handlers = {
    "advance": handle_advance,
    "inspect": handle_inspect,
}

###
# Main
if __name__ == "__main__":
    rollup = Rollup()
    ledger = Ledger()
    accept_previous_request = True

    asset_info = ledger.retrieve_asset()
    EtherId.set(asset_info['asset_id'])

    # Main loop
    while True:
        logger.info("[app] Sending finish")
        next_request_type = rollup.finish(accept_previous_request)
        logger.info(f"[app] Received input of type {next_request_type}")

        accept_previous_request = handlers[next_request_type](rollup,ledger)

    exit(-1)
