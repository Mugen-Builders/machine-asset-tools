from libc.stdint cimport uint64_t, uint8_t
from libc.stdlib cimport free, malloc
cimport libcmt
cimport libcma

cdef class Rollup:
    cdef libcmt.cmt_rollup_t _c_rollup[1]
    cdef libcmt.cmt_abi_address_t *app_address

    def __cinit__(self):
        libcmt.cmt_rollup_init(self._c_rollup)

    def __dealloc__(self):
        libcmt.cmt_rollup_fini(self._c_rollup)

    cpdef str finish(self, bint last_request_status):
        cdef libcmt.cmt_rollup_finish_t finish[1]
        finish[0].accept_previous_request = last_request_status
        err = libcmt.cmt_rollup_finish(self._c_rollup, finish)
        if err != 0:
            raise Exception(f"Failed to finish rollup ({err})")
        if finish[0].next_request_type == libcmt.HTIF_YIELD_REASON_ADVANCE:
            return "advance"
        if finish[0].next_request_type == libcmt.HTIF_YIELD_REASON_INSPECT:
            return "inspect"
        raise Exception(f"Unknown next request type ({finish[0].next_request_type})")

    cpdef object read_advance_state(self):
        cdef libcmt.cmt_rollup_advance_t input[1]
        err = libcmt.cmt_rollup_read_advance_state(self._c_rollup, input)
        if err != 0:
            raise Exception(f"Failed to read advance ({err})")
        app_address = &input[0].app_contract
        payload = input[0].payload.data[:input[0].payload.length]
        app_contract = input[0].app_contract.data[:libcmt.CMT_ABI_ADDRESS_LENGTH]
        msg_sender = input[0].msg_sender.data[:libcmt.CMT_ABI_ADDRESS_LENGTH]
        prev_randao = input[0].prev_randao.data[:libcmt.CMT_ABI_U256_LENGTH]
        ret = dict(input[0])
        ret['payload']['data'] = payload
        ret['app_contract'] = app_contract
        ret['msg_sender'] = msg_sender
        ret['prev_randao'] = prev_randao
        return ret

    cpdef object read_inspect_state(self):
        cdef libcmt.cmt_rollup_inspect_t input[1]
        err = libcmt.cmt_rollup_read_inspect_state(self._c_rollup, input)
        if err != 0:
            raise Exception(f"Failed to read inspect ({err})")
        payload = input[0].payload.data[:input[0].payload.length]
        ret = dict(input[0])
        ret['payload']['data'] = payload
        return ret

    cpdef int emit_voucher(self, str address, object value, bytes payload):
        if address.startswith("0x"):
            address = address[2:]
        cdef libcmt.cmt_abi_address_t address_ptr[1]
        address_ptr[0].data = bytes.fromhex(address[:(2*libcmt.CMT_ABI_ADDRESS_LENGTH)])[:libcmt.CMT_ABI_ADDRESS_LENGTH]
        cdef libcmt.cmt_abi_u256_t value_ptr[1]
        value_ptr[0].data = bytes.fromhex(f"{value:064x}")[:libcmt.CMT_ABI_U256_LENGTH]
        cdef libcmt.cmt_abi_bytes_t payload_ptr[1]
        payload_ptr[0].data = payload
        payload_ptr[0].length = len(payload)
        cdef uint64_t cindex[1]
        err = libcmt.cmt_rollup_emit_voucher(self._c_rollup, address_ptr, value_ptr, payload_ptr, cindex)
        if err != 0:
            raise Exception(f"Failed to emit voucher ({err})")
        return cindex[0]

    cpdef int emit_delegate_call_voucher(self, str address, bytes payload):
        if address.startswith("0x"):
            address = address[2:]
        cdef libcmt.cmt_abi_address_t address_ptr[1]
        address_ptr[0].data = bytes.fromhex(address[:(2*libcmt.CMT_ABI_ADDRESS_LENGTH)])[:libcmt.CMT_ABI_ADDRESS_LENGTH]
        cdef libcmt.cmt_abi_bytes_t payload_ptr[1]
        payload_ptr[0].data = payload
        payload_ptr[0].length = len(payload)
        cdef uint64_t cindex[1]
        err = libcmt.cmt_rollup_emit_delegate_call_voucher(self._c_rollup, address_ptr, payload_ptr, cindex)
        if err != 0:
            raise Exception(f"Failed to emit delegate call voucher ({err})")
        return cindex[0]

    cpdef int emit_notice(self, bytes payload):
        cdef libcmt.cmt_abi_bytes_t payload_ptr[1]
        payload_ptr[0].data = payload
        payload_ptr[0].length = len(payload)
        cdef uint64_t cindex[1]
        err = libcmt.cmt_rollup_emit_notice(self._c_rollup, payload_ptr, cindex)
        if err != 0:
            raise Exception(f"Failed to emit notice ({err})")
        return cindex[0]

    cpdef emit_report(self, bytes payload):
        cdef libcmt.cmt_abi_bytes_t payload_ptr[1]
        payload_ptr[0].data = payload
        payload_ptr[0].length = len(payload)
        err = libcmt.cmt_rollup_emit_report(self._c_rollup, payload_ptr)
        if err != 0:
            raise Exception(f"Failed to emit report ({err})")

    cpdef emit_exception(self, bytes payload):
        cdef libcmt.cmt_abi_bytes_t payload_ptr[1]
        payload_ptr[0].data = payload
        payload_ptr[0].length = len(payload)
        err = libcmt.cmt_rollup_emit_exception(self._c_rollup, payload_ptr)
        if err != 0:
            raise Exception(f"Failed to emit exception ({err})")

    cpdef emit_ether_voucher(self, str receiver, object amount):
        if receiver.startswith("0x"):
            receiver = receiver[2:]
        cdef libcma.cma_parser_voucher_data_t voucher_req[1]
        voucher_req[0].receiver.data = bytes.fromhex(receiver[:(2*libcmt.CMT_ABI_ADDRESS_LENGTH)])[:libcmt.CMT_ABI_ADDRESS_LENGTH]
        voucher_req[0].ether.amount.data = bytes.fromhex(f"{amount:064x}")[:libcmt.CMT_ABI_U256_LENGTH]
        cdef libcma.cma_voucher_t voucher[1]
        voucher[0].payload.data = NULL
        voucher[0].payload.length = 0

        err = libcma.cma_parser_encode_voucher(libcma.CMA_PARSER_VOUCHER_TYPE_ETHER, self.app_address, voucher_req, voucher)
        if err != 0:
            raise Exception(f"Failed to encode ether voucher ({err} - {libcma.cma_parser_get_last_error_message()})")
        self.emit_voucher("0x" + voucher[0].address.data[:libcmt.CMT_ABI_ADDRESS_LENGTH].hex(),
            int.from_bytes(voucher[0].value.data[:libcmt.CMT_ABI_U256_LENGTH]),
            b'')

    cpdef emit_erc20_voucher(self, str token, str receiver, object amount):
        if receiver.startswith("0x"):
            receiver = receiver[2:]
        if token.startswith("0x"):
            token = token[2:]
        cdef libcma.cma_parser_voucher_data_t voucher_req[1]
        voucher_req[0].receiver.data = bytes.fromhex(receiver[:(2*libcmt.CMT_ABI_ADDRESS_LENGTH)])[:libcmt.CMT_ABI_ADDRESS_LENGTH]
        voucher_req[0].erc20.token.data = bytes.fromhex(token[:(2*libcmt.CMT_ABI_ADDRESS_LENGTH)])[:libcmt.CMT_ABI_ADDRESS_LENGTH]
        voucher_req[0].erc20.amount.data = bytes.fromhex(f"{amount:064x}")[:libcmt.CMT_ABI_U256_LENGTH]
        cdef uint8_t voucher_payload_buffer[libcma.CMA_PARSER_ERC20_VOUCHER_PAYLOAD_SIZE]
        cdef libcma.cma_voucher_t voucher[1]
        voucher[0].payload.data = voucher_payload_buffer
        voucher[0].payload.length = sizeof(voucher_payload_buffer)
        err = libcma.cma_parser_encode_voucher(libcma.CMA_PARSER_VOUCHER_TYPE_ERC20, self.app_address, voucher_req, voucher)
        if err != 0:
            raise Exception(f"Failed to encode ether voucher ({err} - {libcma.cma_parser_get_last_error_message()})")
        self.emit_voucher("0x" + voucher[0].address.data[:libcmt.CMT_ABI_ADDRESS_LENGTH].hex(),
            int.from_bytes(voucher[0].value.data[:libcmt.CMT_ABI_U256_LENGTH]),
            voucher[0].payload.data[:voucher[0].payload.length])

cpdef object decode_ether_deposit(dict input):
    cdef libcmt.cmt_rollup_advance_t input_ptr[1]
    input_ptr[0].payload.data = input['payload']['data']
    input_ptr[0].payload.length = input['payload']['length']
    cdef libcma.cma_parser_input_t parser_input[1]
    err = libcma.cma_parser_decode_advance(libcma.CMA_PARSER_INPUT_TYPE_ETHER_DEPOSIT, input_ptr, parser_input);
    if err != 0:
        raise Exception(f"Failed to decode ether deposit ({err} - {libcma.cma_parser_get_last_error_message()})")
    ret = {
        "sender": "0x" + parser_input[0].ether_deposit.sender.data[:libcmt.CMT_ABI_ADDRESS_LENGTH].hex(),
        "amount": int.from_bytes(parser_input[0].ether_deposit.amount.data[:libcmt.CMT_ABI_U256_LENGTH]),
        "exec_layer_data": parser_input[0].ether_deposit.exec_layer_data.data[:parser_input[0].ether_deposit.exec_layer_data.length]
    }
    return ret

cpdef object decode_erc20_deposit(dict input):
    cdef libcmt.cmt_rollup_advance_t input_ptr[1]
    input_ptr[0].payload.data = input['payload']['data']
    input_ptr[0].payload.length = input['payload']['length']
    cdef libcma.cma_parser_input_t parser_input[1]
    err = libcma.cma_parser_decode_advance(libcma.CMA_PARSER_INPUT_TYPE_ERC20_DEPOSIT, input_ptr, parser_input);
    if err != 0:
        raise Exception(f"Failed to decode erc20 deposit ({err} - {libcma.cma_parser_get_last_error_message()})")
    ret = {
        "sender": "0x" + parser_input[0].erc20_deposit.sender.data[:libcmt.CMT_ABI_ADDRESS_LENGTH].hex(),
        "token": "0x" + parser_input[0].erc20_deposit.token.data[:libcmt.CMT_ABI_ADDRESS_LENGTH].hex(),
        "amount": int.from_bytes(parser_input[0].erc20_deposit.amount.data[:libcmt.CMT_ABI_U256_LENGTH]),
        "exec_layer_data": parser_input[0].erc20_deposit.exec_layer_data.data[:parser_input[0].erc20_deposit.exec_layer_data.length]
    }
    return ret

cpdef object decode_advance(dict input):
    cdef libcmt.cmt_rollup_advance_t input_ptr[1]
    input_ptr[0].msg_sender.data = input['msg_sender']
    input_ptr[0].payload.data = input['payload']['data']
    input_ptr[0].payload.length = input['payload']['length']
    cdef libcma.cma_parser_input_t parser_input[1]
    err = libcma.cma_parser_decode_advance(libcma.CMA_PARSER_INPUT_TYPE_AUTO, input_ptr, parser_input);
    if err != 0:
        raise Exception(f"Failed to decode advance ({err} - {libcma.cma_parser_get_last_error_message()})")
    if parser_input[0].type == libcma.CMA_PARSER_INPUT_TYPE_ETHER_WITHDRAWAL:
        ret = {
            "type": "ETHER_WITHDRAWAL",
            "amount": int.from_bytes(parser_input[0].ether_withdrawal.amount.data[:libcmt.CMT_ABI_U256_LENGTH]),
            "exec_layer_data": parser_input[0].ether_withdrawal.exec_layer_data.data[:parser_input[0].ether_withdrawal.exec_layer_data.length]
        }
    elif parser_input[0].type == libcma.CMA_PARSER_INPUT_TYPE_ERC20_WITHDRAWAL:
        ret = {
            "type": "ERC20_WITHDRAWAL",
            "token": "0x" + parser_input[0].erc20_withdrawal.token.data[:libcmt.CMT_ABI_ADDRESS_LENGTH].hex(),
            "amount": int.from_bytes(parser_input[0].erc20_withdrawal.amount.data[:libcmt.CMT_ABI_U256_LENGTH]),
            "exec_layer_data": parser_input[0].erc20_withdrawal.exec_layer_data.data[:parser_input[0].erc20_withdrawal.exec_layer_data.length]
        }
    elif parser_input[0].type == libcma.CMA_PARSER_INPUT_TYPE_ETHER_TRANSFER:
        ret = {
            "type": "ETHER_TRANSFER",
            "receiver": "0x" + parser_input[0].ether_transfer.receiver.data[:libcmt.CMT_ABI_U256_LENGTH].hex(),
            "amount": int.from_bytes(parser_input[0].ether_transfer.amount.data[:libcmt.CMT_ABI_U256_LENGTH]),
            "exec_layer_data": parser_input[0].ether_transfer.exec_layer_data.data[:parser_input[0].ether_transfer.exec_layer_data.length]
        }
    elif parser_input[0].type == libcma.CMA_PARSER_INPUT_TYPE_ERC20_TRANSFER:
        ret = {
            "type": "ERC20_TRANSFER",
            "receiver": "0x" + parser_input[0].erc20_transfer.receiver.data[:libcmt.CMT_ABI_U256_LENGTH].hex(),
            "token": "0x" + parser_input[0].erc20_transfer.token.data[:libcmt.CMT_ABI_ADDRESS_LENGTH].hex(),
            "amount": int.from_bytes(parser_input[0].erc20_transfer.amount.data[:libcmt.CMT_ABI_U256_LENGTH]),
            "exec_layer_data": parser_input[0].erc20_transfer.exec_layer_data.data[:parser_input[0].erc20_transfer.exec_layer_data.length]
        }
    else:
        raise Exception(f"Unknown input type ({parser_input[0].type})")
    return ret

cpdef object decode_inspect(dict input):
    cdef libcmt.cmt_rollup_inspect_t input_ptr[1]
    input_ptr[0].payload.data = input['payload']['data']
    input_ptr[0].payload.length = input['payload']['length']
    cdef libcma.cma_parser_input_t parser_input[1]
    err = libcma.cma_parser_decode_inspect(libcma.CMA_PARSER_INPUT_TYPE_AUTO, input_ptr, parser_input);
    if err != 0:
        raise Exception(f"Failed to decode inspect ({err})")
    if parser_input[0].type == libcma.CMA_PARSER_INPUT_TYPE_BALANCE_ACCOUNT:
        ret = {
            "type": "BALANCE",
            "account": "0x" + parser_input[0].balance.account.data[:libcmt.CMT_ABI_U256_LENGTH].hex(),
            "token": None,
            "token_id": None,
            "exec_layer_data": None
        }
    elif parser_input[0].type == libcma.CMA_PARSER_INPUT_TYPE_BALANCE_ACCOUNT_TOKEN_ADDRESS:
        ret = {
            "type": "BALANCE",
            "account": "0x" + parser_input[0].balance.account.data[:libcmt.CMT_ABI_U256_LENGTH].hex(),
            "token": "0x" + parser_input[0].balance.token.data[:libcmt.CMT_ABI_ADDRESS_LENGTH].hex(),
            "token_id": None,
            "exec_layer_data": None
        }
    elif parser_input[0].type == libcma.CMA_PARSER_INPUT_TYPE_BALANCE_ACCOUNT_TOKEN_ADDRESS_ID:
        ret = {
            "type": "BALANCE",
            "account": "0x" + parser_input[0].balance.account.data[:libcmt.CMT_ABI_U256_LENGTH].hex(),
            "token": "0x" + parser_input[0].balance.token.data[:libcmt.CMT_ABI_ADDRESS_LENGTH].hex(),
            "token_id": int.from_bytes(parser_input[0].balance.token_id[:libcmt.CMT_ABI_U256_LENGTH].data),
            "exec_layer_data": parser_input[0].balance.exec_layer_data.data[:parser_input[0].balance.exec_layer_data.length] if parser_input[0].balance.exec_layer_data.length > 0 else None
        }
    elif parser_input[0].type == libcma.CMA_PARSER_INPUT_TYPE_SUPPLY:
        ret = {
            "type": "SUPPLY",
            "token": None,
            "token_id": None,
            "exec_layer_data": None
        }
    elif parser_input[0].type == libcma.CMA_PARSER_INPUT_TYPE_SUPPLY_TOKEN_ADDRESS:
        ret = {
            "type": "SUPPLY",
            "token": "0x" + parser_input[0].supply.token.data[:libcmt.CMT_ABI_ADDRESS_LENGTH].hex(),
            "token_id": None,
            "exec_layer_data": None
        }
    elif parser_input[0].type == libcma.CMA_PARSER_INPUT_TYPE_SUPPLY_TOKEN_ADDRESS_ID:
        ret = {
            "type": "SUPPLY",
            "token": "0x" + parser_input[0].supply.token.data[:libcmt.CMT_ABI_ADDRESS_LENGTH].hex(),
            "token_id": int.from_bytes(parser_input[0].supply.token_id.data[:libcmt.CMT_ABI_U256_LENGTH]),
            "exec_layer_data": parser_input[0].supply.exec_layer_data.data[:parser_input[0].supply.exec_layer_data.length] if parser_input[0].supply.exec_layer_data.length > 0 else None
        }
    else:
        raise Exception(f"Unknown input type ({parser_input[0].type})")
    return ret

cdef class Ledger:
    cdef libcma.cma_ledger_t _c_ledger[1]

    def __cinit__(self):
        libcma.cma_ledger_init(self._c_ledger)

    def __dealloc__(self):
        libcma.cma_ledger_fini(self._c_ledger)

    cpdef reset(self):
        err = libcma.cma_ledger_reset(self._c_ledger)
        if err != 0:
            raise Exception(f"Failed to reset ledger ({err} - {libcma.cma_ledger_get_last_error_message()})")

    def retrieve_asset(self, asset_id = None, token = None, token_id = None):
        cdef libcma.cma_ledger_asset_id_t lassid[1]
        cdef libcma.cma_token_address_t ltok[1]
        cdef libcma.cma_token_id_t ltokid[1]
        cdef libcma.cma_ledger_asset_type_t asset_type[1]
        asset_type[0] = libcma.CMA_LEDGER_ASSET_TYPE_ID
        cdef libcma.cma_ledger_retrieve_operation_t operation = libcma.CMA_LEDGER_OP_FIND_OR_CREATE
        if asset_id is not None:
            lassid[0] = asset_id
        elif token is not None:
            if token.startswith("0x"):
                token = token[2:]
            asset_type[0] = libcma.CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS
            ltok[0].data = bytes.fromhex(token[:(2*libcmt.CMT_ABI_ADDRESS_LENGTH)])[:libcmt.CMT_ABI_ADDRESS_LENGTH]
            if token_id is not None:
                if token_id.startswith("0x"):
                    token_id = token_id[2:]
                ltokid[0].data = bytes.fromhex(token_id[:(2*libcmt.CMT_ABI_U256_LENGTH)])[:libcmt.CMT_ABI_U256_LENGTH]
                asset_type[0] = libcma.CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID
        else:
            operation = libcma.CMA_LEDGER_OP_CREATE
        err = libcma.cma_ledger_retrieve_asset(self._c_ledger, lassid, ltok, ltokid, asset_type, operation)
        if err != 0:
            raise Exception(f"Failed to retrieve asset ({err} - {libcma.cma_ledger_get_last_error_message()})")
        token_str = None
        token_id_str = None
        if asset_type[0] == libcma.CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS:
            token_str = "0x" + ltok[0].data[:libcmt.CMT_ABI_ADDRESS_LENGTH].hex()
        elif asset_type[0] == libcma.CMA_LEDGER_ASSET_TYPE_TOKEN_ADDRESS_ID:
            token_id_str = int.from_bytes(ltokid[0].data.hex()[:libcmt.CMT_ABI_U256_LENGTH])
        return {
            "asset_id": lassid[0],
            "token": token_str,
            "token_id": token_id_str
        }

    def retrieve_account(self, account_id = None, account = None):
        cdef libcma.cma_ledger_account_id_t laccid[1]
        cdef libcma.cma_ledger_account_t lacc[1]
        cdef libcma.cma_ledger_account_type_t account_type[1]
        account_type[0] = libcma.CMA_LEDGER_ACCOUNT_TYPE_ID
        cdef libcma.cma_ledger_retrieve_operation_t operation = libcma.CMA_LEDGER_OP_FIND_OR_CREATE
        if account_id is not None:
            laccid[0] = account_id
        elif account is not None:
            if account.startswith("0x"):
                account = account[2:]
            if len(account) == 2*libcmt.CMT_ABI_ADDRESS_LENGTH:
                account_type[0] = libcma.CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS
                lacc[0].address.data = bytes.fromhex(account[:(2*libcmt.CMT_ABI_ADDRESS_LENGTH)])[:libcmt.CMT_ABI_ADDRESS_LENGTH]
            elif len(account) == 2*libcmt.CMT_ABI_U256_LENGTH:
                account_type[0] = libcma.CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID
                lacc[0].account_id.data = bytes.fromhex(account[:(2*libcmt.CMT_ABI_U256_LENGTH)])[:libcmt.CMT_ABI_U256_LENGTH]
            else:
                raise ValueError(f"Invalid account address length: {len(account)}")
        else:
            account_type[0] = libcma.CMA_LEDGER_ACCOUNT_TYPE_ID
            operation = libcma.CMA_LEDGER_OP_CREATE
        err = libcma.cma_ledger_retrieve_account(self._c_ledger, laccid, lacc, NULL, account_type, operation)
        if err != 0:
            raise Exception(f"Failed to retrieve account ({err} - {libcma.cma_ledger_get_last_error_message()})")
        account_str = None
        if lacc[0].type == libcma.CMA_LEDGER_ACCOUNT_TYPE_WALLET_ADDRESS:
            account_str = "0x" + lacc[0].address.data[:libcmt.CMT_ABI_ADDRESS_LENGTH].hex()
        elif lacc[0].type == libcma.CMA_LEDGER_ACCOUNT_TYPE_ACCOUNT_ID:
            account_str = "0x" + lacc[0].account_id.data[:libcmt.CMT_ABI_U256_LENGTH].hex()
        return {
            "account_id": laccid[0],
            "account": account_str
        }

    cpdef deposit(self, libcma.cma_ledger_asset_id_t asset_id, libcma.cma_ledger_account_id_t account_id, object amount):
        cdef libcma.cma_amount_t lamount[1]
        lamount[0].data = bytes.fromhex(f"{amount:064x}")[:libcmt.CMT_ABI_U256_LENGTH]
        err = libcma.cma_ledger_deposit(self._c_ledger, asset_id, account_id, lamount)
        if err != 0:
            raise Exception(f"Failed to deposit ({err} - {libcma.cma_ledger_get_last_error_message()})")

    cpdef withdraw(self, libcma.cma_ledger_asset_id_t asset_id, libcma.cma_ledger_account_id_t account_id, object amount):
        cdef libcma.cma_amount_t lamount[1]
        lamount[0].data = bytes.fromhex(f"{amount:064x}")[:libcmt.CMT_ABI_U256_LENGTH]
        err = libcma.cma_ledger_withdraw(self._c_ledger, asset_id, account_id, lamount)
        if err != 0:
            raise Exception(f"Failed to withdraw ({err} - {libcma.cma_ledger_get_last_error_message()})")

    cpdef transfer(self, libcma.cma_ledger_asset_id_t asset_id, libcma.cma_ledger_account_id_t from_account_id, libcma.cma_ledger_account_id_t to_account_id, object amount):
        cdef libcma.cma_amount_t lamount[1]
        lamount[0].data = bytes.fromhex(f"{amount:064x}")[:libcmt.CMT_ABI_U256_LENGTH]
        err = libcma.cma_ledger_transfer(self._c_ledger, asset_id, from_account_id, to_account_id, lamount)
        if err != 0:
            raise Exception(f"Failed to transfer ({err} - {libcma.cma_ledger_get_last_error_message()})")

    cpdef object balance(self, libcma.cma_ledger_asset_id_t asset_id, libcma.cma_ledger_account_id_t account_id):
        cdef libcma.cma_amount_t lamount[1]
        err = libcma.cma_ledger_get_balance(self._c_ledger, asset_id, account_id, lamount)
        if err != 0:
            raise Exception(f"Failed to get balance ({err} - {libcma.cma_ledger_get_last_error_message()})")
        return int.from_bytes(lamount[0].data[:libcmt.CMT_ABI_U256_LENGTH])

    cpdef object supply(self, libcma.cma_ledger_asset_id_t asset_id):
        cdef libcma.cma_amount_t lamount[1]
        err = libcma.cma_ledger_get_total_supply(self._c_ledger, asset_id, lamount)
        if err != 0:
            raise Exception(f"Failed to get total supply ({err} - {libcma.cma_ledger_get_last_error_message()})")
        return int.from_bytes(lamount[0].data[:libcmt.CMT_ABI_U256_LENGTH])
