from libc.stdint cimport uint64_t, uint8_t

cdef extern from "/usr/include/libcmt/io.h":
    cdef enum:
        HTIF_YIELD_REASON_ADVANCE
        HTIF_YIELD_REASON_INSPECT

cdef extern from "/usr/include/libcmt/abi.h":
    cdef enum:
        CMT_ABI_U256_LENGTH
        CMT_ABI_ADDRESS_LENGTH
    ctypedef struct cmt_abi_address_t:
        uint8_t data[CMT_ABI_ADDRESS_LENGTH]
    ctypedef struct cmt_abi_u256_t:
        uint8_t data[CMT_ABI_U256_LENGTH]
    ctypedef struct cmt_abi_bytes_t:
        uint8_t *data
        uint64_t length

cdef extern from "/usr/include/libcmt/rollup.h":
    ctypedef struct cmt_rollup_t:
        pass
    ctypedef struct cmt_rollup_advance_t:
        uint64_t chain_id
        cmt_abi_address_t app_contract
        cmt_abi_address_t msg_sender
        uint64_t block_number
        uint64_t block_timestamp
        cmt_abi_u256_t prev_randao
        uint64_t index
        cmt_abi_bytes_t payload
    ctypedef struct cmt_rollup_inspect_t:
        cmt_abi_bytes_t payload
    ctypedef struct cmt_rollup_finish_t:
        bint accept_previous_request
        int next_request_type
    # ctypedef struct cmt_gio_t:
    #     pass

    int cmt_rollup_init(cmt_rollup_t* me)
    int cmt_rollup_fini(cmt_rollup_t* me)

    int cmt_rollup_emit_voucher(cmt_rollup_t *me, cmt_abi_address_t *address, cmt_abi_u256_t *value, const cmt_abi_bytes_t *data, uint64_t *index)
    int cmt_rollup_emit_delegate_call_voucher(cmt_rollup_t *me, cmt_abi_address_t *address, cmt_abi_bytes_t *data, uint64_t *index)
    int cmt_rollup_emit_notice(cmt_rollup_t *me, cmt_abi_bytes_t *payload, uint64_t *index)
    int cmt_rollup_emit_report(cmt_rollup_t *me, cmt_abi_bytes_t *payload)
    int cmt_rollup_emit_exception(cmt_rollup_t *me, cmt_abi_bytes_t *payload)

    int cmt_rollup_read_advance_state(cmt_rollup_t *me, cmt_rollup_advance_t *advance)
    int cmt_rollup_read_inspect_state(cmt_rollup_t *me, cmt_rollup_inspect_t *inspect)
    int cmt_rollup_finish(cmt_rollup_t *me, cmt_rollup_finish_t *finish)

    # int cmt_gio_request(cmt_rollup_t *me, cmt_gio_t *req)
    # int cmt_rollup_load_merkle(cmt_rollup_t *me, char *path)
    # int cmt_rollup_save_merkle(cmt_rollup_t *me, char *path)
    # void cmt_rollup_reset_merkle(cmt_rollup_t *me)
