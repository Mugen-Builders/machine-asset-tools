import sys
import os
current_dir = os.path.realpath(os.path.dirname(os.path.abspath(__file__)))
parent_dir = os.path.dirname(current_dir)
sys.path.append(parent_dir)

import pytest
import json

from cartesi import abi
from cartesi.models import ABIFunctionSelectorHeader

from cartesapp.utils import hex2bytes, bytes2hex, str2hex
from cartesapp.testclient import TestClient
from model import DepositEtherPayload, ETHER_PORTAL_ADDRESS, \
    DepositErc20Payload, ERC20_PORTAL_ADDRESS, Erc20Voucher, \
    DepositErc721Payload, ERC721_PORTAL_ADDRESS, Erc721Voucher, DataBytes, \
    DepositErc1155SinglePayload, ERC1155_SINGLE_PORTAL_ADDRESS, Erc1155SingleVoucher, \
    DepositErc1155BatchPayload, ERC1155_BATCH_PORTAL_ADDRESS, Erc1155BatchVoucher, BatchBytes


AMOUNT = 10_000_000_000_000_000

USER1_ADDRESS = f"{1000:#042x}"
USER2_ADDRESS = f"{1001:#042x}"
TOKEN_ADDRESS = f"{1234:#042x}"
TOKEN_ID1 = 1
TOKEN_ID2 = 2

###
# Tests

# test application setup
@pytest.fixture(scope='session')
def app_client() -> TestClient:
    client = TestClient()
    return client

@pytest.fixture()
def ether_deposit_payload() -> DepositEtherPayload:
    return DepositEtherPayload(
        sender= USER1_ADDRESS,
        amount=AMOUNT,
        exec_layer_data=b'Deposited Ether'
    )

def test_should_deposit_ether(
        app_client: TestClient,
        ether_deposit_payload: DepositEtherPayload):

    hex_payload = bytes2hex(abi.encode_model(ether_deposit_payload,True))
    app_client.send_advance(
        msg_sender=ETHER_PORTAL_ADDRESS,
        hex_payload=hex_payload
    )

    assert app_client.rollup.status

    voucher_address = app_client.rollup.vouchers[-1]['data']['destination']
    assert voucher_address == ether_deposit_payload.sender
    voucher_value = app_client.rollup.vouchers[-1]['data']['value']
    assert voucher_value == ether_deposit_payload.amount

@pytest.fixture()
def erc20_deposit_payload() -> DepositErc20Payload:
    return DepositErc20Payload(
        sender = USER1_ADDRESS,
        token = TOKEN_ADDRESS,
        amount=AMOUNT,
        exec_layer_data=b'Deposited Erc20'
    )

def test_should_deposit_erc20(
        app_client: TestClient,
        erc20_deposit_payload: DepositErc20Payload):

    hex_payload = bytes2hex(abi.encode_model(erc20_deposit_payload,True))
    app_client.send_advance(
        msg_sender=ERC20_PORTAL_ADDRESS,
        hex_payload=hex_payload
    )

    assert app_client.rollup.status

    voucher_address = app_client.rollup.vouchers[-1]['data']['destination']
    assert voucher_address == erc20_deposit_payload.token
    voucher = app_client.rollup.vouchers[-1]['data']['payload']
    voucher_bytes = hex2bytes(voucher)
    voucher_model = abi.decode_to_model(data=voucher_bytes[4:],model=Erc20Voucher)
    assert voucher_model.amount == erc20_deposit_payload.amount
    assert voucher_model.receiver == erc20_deposit_payload.sender

@pytest.fixture()
def erc721_deposit_payload() -> DepositErc721Payload:
    data_bytes = DataBytes(
        base_layer_data=b'',
        exec_layer_data=b''
    )
    return DepositErc721Payload(
        sender = USER1_ADDRESS,
        token = TOKEN_ADDRESS,
        token_id=TOKEN_ID1,
        data_bytes=abi.encode_model(data_bytes)
    )

def test_should_deposit_erc721(
        app_client: TestClient,
        erc721_deposit_payload: DepositErc721Payload):

    hex_payload = bytes2hex(abi.encode_model(erc721_deposit_payload,True))
    app_client.send_advance(
        msg_sender=ERC721_PORTAL_ADDRESS,
        hex_payload=hex_payload
    )

    assert app_client.rollup.status

    voucher = app_client.rollup.vouchers[-1]['data']['payload']
    voucher_bytes = hex2bytes(voucher)
    voucher_model = abi.decode_to_model(data=voucher_bytes[4:],model=Erc721Voucher)
    assert voucher_model.token_id == erc721_deposit_payload.token_id

@pytest.fixture()
def erc1155_single_deposit_payload() -> DepositErc1155SinglePayload:
    data_bytes = DataBytes(
        base_layer_data=b'',
        exec_layer_data=b''
    )
    return DepositErc1155SinglePayload(
        sender = USER1_ADDRESS,
        token = TOKEN_ADDRESS,
        token_id=TOKEN_ID1,
        amount=10*AMOUNT,
        data_bytes=abi.encode_model(data_bytes)
    )

def test_should_deposit_erc1155_single(
        app_client: TestClient,
        erc1155_single_deposit_payload: DepositErc1155SinglePayload):

    hex_payload = bytes2hex(abi.encode_model(erc1155_single_deposit_payload,True))
    app_client.send_advance(
        msg_sender=ERC1155_SINGLE_PORTAL_ADDRESS,
        hex_payload=hex_payload
    )

    assert app_client.rollup.status

    voucher = app_client.rollup.vouchers[-1]['data']['payload']
    voucher_bytes = hex2bytes(voucher)
    voucher_model = abi.decode_to_model(data=voucher_bytes[4:],model=Erc1155SingleVoucher)
    assert voucher_model.token_id == erc1155_single_deposit_payload.token_id
    assert voucher_model.amount == erc1155_single_deposit_payload.amount

@pytest.fixture()
def erc1155_batch_deposit_payload() -> DepositErc1155BatchPayload:
    batch_bytes = BatchBytes(
        token_ids=[TOKEN_ID1,TOKEN_ID2],
        amounts=[10*AMOUNT,5*AMOUNT],
        base_layer_data=b'',
        exec_layer_data=b''
    )
    return DepositErc1155BatchPayload(
        sender = USER1_ADDRESS,
        token = TOKEN_ADDRESS,
        batch_bytes=abi.encode_model(batch_bytes)
    )

def test_should_deposit_erc1155_batch(
        app_client: TestClient,
        erc1155_batch_deposit_payload: DepositErc1155BatchPayload):

    hex_payload = bytes2hex(abi.encode_model(erc1155_batch_deposit_payload,True))
    app_client.send_advance(
        msg_sender=ERC1155_BATCH_PORTAL_ADDRESS,
        hex_payload=hex_payload
    )

    assert app_client.rollup.status

    voucher = app_client.rollup.vouchers[-1]['data']['payload']
    voucher_bytes = hex2bytes(voucher)
    voucher_model = abi.decode_to_model(data=voucher_bytes[4:],model=Erc1155BatchVoucher)
    batch = abi.decode_to_model(data=erc1155_batch_deposit_payload.batch_bytes,model=BatchBytes)
    assert len(voucher_model.token_ids) == len(batch.token_ids)
    assert voucher_model.token_ids[0] == batch.token_ids[0]
    assert voucher_model.token_ids[1] == batch.token_ids[1]
    assert len(voucher_model.amounts) == len(batch.amounts)
    assert voucher_model.amounts[0] == batch.amounts[0]
    assert voucher_model.amounts[1] == batch.amounts[1]
