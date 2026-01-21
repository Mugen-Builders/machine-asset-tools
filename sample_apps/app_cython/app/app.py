
import logging
import os
import traceback
from cmpy import Rollup, decode_advance, decode_inspect, decode_ether_deposit, decode_erc20_deposit, \
    decode_erc721_deposit, decode_erc1155_single_deposit, decode_erc1155_batch_deposit, \
    Ledger

ETHER_PORTAL_ADDRESS = "0xA632c5c05812c6a6149B7af5C56117d1D2603828"[2:].lower()
ERC20_PORTAL_ADDRESS = "0xACA6586A0Cf05bD831f2501E7B4aea550dA6562D"[2:].lower()
ERC721_PORTAL_ADDRESS = "0x9E8851dadb2b77103928518846c4678d48b5e371"[2:].lower()
ERC1155_SINGLE_PORTAL_ADDRESS = "0x18558398Dd1a8cE20956287a4Da7B76aE7A96662"[2:].lower()
ERC1155_BATCH_PORTAL_ADDRESS = "0xe246Abb974B307490d9C6932F48EbE79de72338A"[2:].lower()

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

        asset_info = ledger.retrieve_asset(token=deposit['token'])
        account_info = ledger.retrieve_account(account=deposit['sender'])

        ledger.deposit(asset_info['asset_id'], account_info['account_id'], deposit['amount'])
        logger.info(f"[app] {deposit['sender']} deposited {deposit['amount']} of token {asset_info['token']}")
        return True

    if msg_sender == ERC721_PORTAL_ADDRESS:
        deposit = decode_erc721_deposit(advance)

        asset_info = ledger.retrieve_asset(token=deposit['token'],token_id=deposit['token_id'])
        account_info = ledger.retrieve_account(account=deposit['sender'])

        ledger.deposit(asset_info['asset_id'], account_info['account_id'], 1)
        logger.info(f"[app] {deposit['sender']} deposited id {deposit['token_id']} from token {asset_info['token']}")
        return True

    if msg_sender == ERC1155_SINGLE_PORTAL_ADDRESS:
        deposit = decode_erc1155_single_deposit(advance)

        asset_info = ledger.retrieve_asset(token=deposit['token'],token_id=deposit['token_id'])
        account_info = ledger.retrieve_account(account=deposit['sender'])

        ledger.deposit(asset_info['asset_id'], account_info['account_id'], deposit['amount'])
        logger.info(f"[app] {deposit['sender']} deposited {deposit['amount']} of id {deposit['token_id']} from token {asset_info['token']}")
        return True

    if msg_sender == ERC1155_BATCH_PORTAL_ADDRESS:
        deposit = decode_erc1155_batch_deposit(advance)

        account_info = ledger.retrieve_account(account=deposit['sender'])
        for i in range(len(deposit['token_ids'])):
            asset_info = ledger.retrieve_asset(token=deposit['token'],token_id=deposit['token_ids'][i])

            ledger.deposit(asset_info['asset_id'], account_info['account_id'], deposit['amounts'][i])
            logger.info(f"[app] {deposit['sender']} deposited {deposit['amounts'][i]} of id {deposit['token_ids'][i]} from token {asset_info['token']}")
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

        if decoded_advance['type'] == 'ERC721_WITHDRAWAL':
            asset_info = ledger.retrieve_asset(token=decoded_advance['token'],token_id=decoded_advance['token_id'])
            account_info = ledger.retrieve_account(account=msg_sender)

            ledger.withdraw(asset_info['asset_id'], account_info['account_id'], 1)
            logger.info(f"[app] {msg_sender} withdrew id {decoded_advance['token_id']} from token {asset_info['token']}")

            rollup.emit_erc721_voucher(asset_info['token'], msg_sender, decoded_advance['token_id'])
            logger.info("[app] Erc721 voucher emitted")
            return True

        if decoded_advance['type'] == 'ERC1155_SINGLE_WITHDRAWAL':
            asset_info = ledger.retrieve_asset(token=decoded_advance['token'],token_id=decoded_advance['token_id'])
            account_info = ledger.retrieve_account(account=msg_sender)

            ledger.withdraw(asset_info['asset_id'], account_info['account_id'], decoded_advance['amount'])
            logger.info(f"[app] {msg_sender} withdrew {decoded_advance['amount']} of id {decoded_advance['token_id']} from token {asset_info['token']}")

            rollup.emit_erc1155_single_voucher(asset_info['token'], msg_sender, decoded_advance['token_id'], decoded_advance['amount'])
            logger.info("[app] Erc1155_single voucher emitted")
            return True

        if decoded_advance['type'] == 'ERC1155_BATCH_WITHDRAWAL':
            account_info = ledger.retrieve_account(account=msg_sender)

            for i in range(len(decoded_advance['token_ids'])):
                asset_info = ledger.retrieve_asset(token=decoded_advance['token'],token_id=decoded_advance['token_ids'][i])
                ledger.withdraw(asset_info['asset_id'], account_info['account_id'], decoded_advance['amounts'][i])
                logger.info(f"[app] {msg_sender} withdrew {decoded_advance['amounts'][i]} of id {decoded_advance['token_ids'][i]} from token {asset_info['token']}")

            rollup.emit_erc1155_batch_voucher(asset_info['token'], msg_sender, decoded_advance['token_ids'], decoded_advance['amounts'])
            logger.info("[app] Erc1155_batch voucher emitted")
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

        if decoded_advance['type'] == 'ERC721_TRANSFER':
            asset_info = ledger.retrieve_asset(token=decoded_advance['token'],token_id=decoded_advance['token_id'])
            account_info_from = ledger.retrieve_account(account=msg_sender)
            account_info_to = ledger.retrieve_account(account=decoded_advance['receiver'])

            ledger.transfer(asset_info['asset_id'], account_info_from['account_id'], account_info_to['account_id'], 1)
            logger.info(f"[app] {msg_sender} transfered to {account_info_to['account']} id {decoded_advance['token_id']} from token {asset_info['token']}")
            return True

        if decoded_advance['type'] == 'ERC1155_SINGLE_TRANSFER':
            asset_info = ledger.retrieve_asset(token=decoded_advance['token'],token_id=decoded_advance['token_id'])
            account_info_from = ledger.retrieve_account(account=msg_sender)
            account_info_to = ledger.retrieve_account(account=decoded_advance['receiver'])

            ledger.transfer(asset_info['asset_id'], account_info_from['account_id'], account_info_to['account_id'], decoded_advance['amount'])
            logger.info(f"[app] {msg_sender} transfered to {account_info_to['account']} {decoded_advance['amount']} of id {decoded_advance['token_id']} from token {asset_info['token']}")
            return True

        if decoded_advance['type'] == 'ERC1155_BATCH_TRANSFER':
            account_info_from = ledger.retrieve_account(account=msg_sender)
            account_info_to = ledger.retrieve_account(account=decoded_advance['receiver'])

            for i in range(len(decoded_advance['token_ids'])):
                asset_info = ledger.retrieve_asset(token=decoded_advance['token'],token_id=decoded_advance['token_ids'][i])

                ledger.transfer(asset_info['asset_id'], account_info_from['account_id'], account_info_to['account_id'], decoded_advance['amounts'][i])
                logger.info(f"[app] {msg_sender} transfered to {account_info_to['account']} {decoded_advance['amounts'][i]} of id {decoded_advance['token_ids'][i]} from token {asset_info['token']}")
            return True

        logger.info("[app] unidentified wallet input")
        return False
    except Exception as e:
        logger.error(f"[app] Failed to process advance: {e}")
        logger.error(traceback.format_exc())
        return False

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
