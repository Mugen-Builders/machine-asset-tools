
typedef struct {
    uint8_t *begin;
    uint8_t *end;
} cmt_buf_t;

void cmt_buf_init(cmt_buf_t *me, size_t length, void *data);

int cmt_buf_split(const cmt_buf_t *me, size_t lhs_length, cmt_buf_t *lhs, cmt_buf_t *rhs);

size_t cmt_buf_length(const cmt_buf_t *me);

void cmt_buf_xxd(void *begin, void *end, int bytes_per_line);

bool cmt_buf_split_by_comma(cmt_buf_t *x, cmt_buf_t *xs);



enum {
    CMT_ABI_U256_LENGTH = 32,
    CMT_ABI_ADDRESS_LENGTH = 20,
};



typedef struct cmt_abi_address {
    uint8_t data[CMT_ABI_ADDRESS_LENGTH];
} cmt_abi_address_t;


typedef struct cmt_abi_u256 {
    uint8_t data[CMT_ABI_U256_LENGTH];
} cmt_abi_u256_t;

typedef struct cmt_abi_bytes {
    size_t length;
    void *data;
} cmt_abi_bytes_t;

uint32_t cmt_abi_funsel(uint8_t a, uint8_t b, uint8_t c, uint8_t d);

int cmt_abi_mark_frame(const cmt_buf_t *me, cmt_buf_t *frame);

// put section ---------------------------------------------------------------

int cmt_abi_put_funsel(cmt_buf_t *me, uint32_t funsel);

int cmt_abi_put_uint(cmt_buf_t *me, size_t data_length, const void *data);

int cmt_abi_put_uint_be(cmt_buf_t *me, size_t data_length, const void *data);

int cmt_abi_put_uint256(cmt_buf_t *me, const cmt_abi_u256_t *value);

int cmt_abi_put_bool(cmt_buf_t *me, bool value);

int cmt_abi_put_address(cmt_buf_t *me, const cmt_abi_address_t *address);

int cmt_abi_put_bytes_s(cmt_buf_t *me, cmt_buf_t *offset);

//int cmt_abi_put_bytes_d(cmt_buf_t *me, cmt_buf_t *offset, size_t n, const void *data, const void *start);
int cmt_abi_put_bytes_d(cmt_buf_t *me, cmt_buf_t *offset, const cmt_buf_t *frame, const cmt_abi_bytes_t *payload);

int cmt_abi_reserve_bytes_d(cmt_buf_t *me, cmt_buf_t *of, size_t n, cmt_buf_t *out, const void *start);

// get section ---------------------------------------------------------------

uint32_t cmt_abi_peek_funsel(cmt_buf_t *me);

int cmt_abi_check_funsel(cmt_buf_t *me, uint32_t expected);

int cmt_abi_get_uint256(cmt_buf_t *me, cmt_abi_u256_t *value);

int cmt_abi_get_uint(cmt_buf_t *me, size_t n, void *data);

int cmt_abi_get_uint_be(cmt_buf_t *me, size_t n, void *data);

int cmt_abi_get_bool(cmt_buf_t *me, bool *value);

int cmt_abi_get_address(cmt_buf_t *me, cmt_abi_address_t *value);

int cmt_abi_get_bytes_s(cmt_buf_t *me, cmt_buf_t of[1]);

int cmt_abi_get_bytes_d(const cmt_buf_t *start, cmt_buf_t of[1], size_t *n, void **data);

int cmt_abi_peek_bytes_d(const cmt_buf_t *start, cmt_buf_t of[1], cmt_buf_t *bytes);

// raw codec section --------------------------------------------------------

int cmt_abi_encode_uint(size_t n, const void *data, uint8_t out[CMT_ABI_U256_LENGTH]);

int cmt_abi_encode_uint_nr(size_t n, const uint8_t *data, uint8_t out[CMT_ABI_U256_LENGTH]);

int cmt_abi_encode_uint_nn(size_t n, const uint8_t *data, uint8_t out[CMT_ABI_U256_LENGTH]);

int cmt_abi_decode_uint(const uint8_t data[CMT_ABI_U256_LENGTH], size_t n, uint8_t *out);

int cmt_abi_decode_uint_nr(const uint8_t data[CMT_ABI_U256_LENGTH], size_t n, uint8_t *out);

int cmt_abi_decode_uint_nn(const uint8_t data[CMT_ABI_U256_LENGTH], size_t n, uint8_t *out);



enum {
    CMT_KECCAK_LENGTH = 32,
};


typedef union cmt_keccak_state {
    uint8_t b[200];
    uint64_t q[25];
} cmt_keccak_state_t;

typedef struct cmt_keccak {
    cmt_keccak_state_t st;
    int pt, rsiz;
} cmt_keccak_t;

void cmt_keccak_init(cmt_keccak_t *state);

void cmt_keccak_update(cmt_keccak_t *state, size_t n, const void *data);

void cmt_keccak_final(cmt_keccak_t *state, void *md);

uint8_t *cmt_keccak_data(size_t length, const void *data, uint8_t md[CMT_KECCAK_LENGTH]);

uint32_t cmt_keccak_funsel(const char *decl);


enum {
    CMT_MERKLE_TREE_HEIGHT = 63,
};

typedef struct {
    uint64_t leaf_count;
    uint8_t state[CMT_MERKLE_TREE_HEIGHT][CMT_KECCAK_LENGTH];
} cmt_merkle_t;

void cmt_merkle_init(cmt_merkle_t *me);

void cmt_merkle_reset(cmt_merkle_t *me);

void cmt_merkle_fini(cmt_merkle_t *me);

int cmt_merkle_load(cmt_merkle_t *me, const char *filepath);

int cmt_merkle_save(cmt_merkle_t *me, const char *filepath);

uint64_t cmt_merkle_get_leaf_count(cmt_merkle_t *me);

int cmt_merkle_push_back(cmt_merkle_t *me, const uint8_t hash[CMT_KECCAK_LENGTH]);

int cmt_merkle_push_back_data(cmt_merkle_t *me, size_t length, const void *data);

void cmt_merkle_get_root_hash(cmt_merkle_t *me, uint8_t root[CMT_KECCAK_LENGTH]);



enum {
    HTIF_DEVICE_YIELD = 2,
};


enum {
    HTIF_YIELD_CMD_AUTOMATIC = 0,
    HTIF_YIELD_CMD_MANUAL = 1,
};


enum {
    HTIF_YIELD_AUTOMATIC_REASON_PROGRESS = 1,
    HTIF_YIELD_AUTOMATIC_REASON_TX_OUTPUT = 2,
    HTIF_YIELD_AUTOMATIC_REASON_TX_REPORT = 4,
};


enum {
    HTIF_YIELD_MANUAL_REASON_RX_ACCEPTED = 1,
    HTIF_YIELD_MANUAL_REASON_RX_REJECTED = 2,
    HTIF_YIELD_MANUAL_REASON_TX_EXCEPTION = 4,
};


enum {
    HTIF_YIELD_REASON_ADVANCE = 0,
    HTIF_YIELD_REASON_INSPECT = 1,
};

typedef struct {
    cmt_buf_t tx[1];
    cmt_buf_t rx[1];
    int fd;
} cmt_io_driver_ioctl_t;

typedef struct {
    cmt_buf_t tx[1];
    cmt_buf_t rx[1];
    cmt_buf_t inputs_left;

    int input_type;
    char input_filename[128];
    char input_fileext[16];

    int input_seq;
    int output_seq;
    int report_seq;
    int exception_seq;
    int gio_seq;
} cmt_io_driver_mock_t;


typedef union cmt_io_driver {
    cmt_io_driver_ioctl_t ioctl;
    cmt_io_driver_mock_t mock;
} cmt_io_driver_t;


typedef struct cmt_io_yield {
    uint8_t dev;
    uint8_t cmd;
    uint16_t reason;
    uint32_t data;
} cmt_io_yield_t;

int cmt_io_init(cmt_io_driver_t *me);

void cmt_io_fini(cmt_io_driver_t *me);

cmt_buf_t cmt_io_get_tx(cmt_io_driver_t *me);

cmt_buf_t cmt_io_get_rx(cmt_io_driver_t *me);

int cmt_io_yield(cmt_io_driver_t *me, cmt_io_yield_t *rr);


typedef struct cmt_rollup {
    union cmt_io_driver io[1];
    uint32_t fromhost_data;
    cmt_merkle_t merkle[1];
} cmt_rollup_t;


typedef struct cmt_rollup_advance {
    uint64_t chain_id;
    cmt_abi_address_t app_contract;
    cmt_abi_address_t msg_sender;
    uint64_t block_number;
    uint64_t block_timestamp;
    cmt_abi_u256_t prev_randao;
    uint64_t index;
    cmt_abi_bytes_t payload;
} cmt_rollup_advance_t;


typedef struct cmt_rollup_inspect {
    cmt_abi_bytes_t payload;
} cmt_rollup_inspect_t;


typedef struct cmt_rollup_finish_s {
    bool accept_previous_request;
    int next_request_type;
    uint32_t next_request_payload_length;
} cmt_rollup_finish_t;


typedef struct cmt_gio {
    uint16_t domain;
    uint32_t id_length;
    void *id;
    uint16_t response_code;
    uint32_t response_data_length;
    void *response_data;
} cmt_gio_t;

int cmt_rollup_init(cmt_rollup_t *me);

void cmt_rollup_fini(cmt_rollup_t *me);

int cmt_rollup_emit_voucher(cmt_rollup_t *me, const cmt_abi_address_t *address, const cmt_abi_u256_t *value, const cmt_abi_bytes_t *data, uint64_t *index);

int cmt_rollup_emit_delegate_call_voucher(cmt_rollup_t *me, const cmt_abi_address_t *address, const cmt_abi_bytes_t *data, uint64_t *index);

int cmt_rollup_emit_notice(cmt_rollup_t *me, const cmt_abi_bytes_t *payload, uint64_t *index);

int cmt_rollup_emit_report(cmt_rollup_t *me, const cmt_abi_bytes_t *payload);

int cmt_rollup_emit_exception(cmt_rollup_t *me, const cmt_abi_bytes_t *payload);

int cmt_rollup_progress(cmt_rollup_t *me, uint32_t progress);

int cmt_rollup_read_advance_state(cmt_rollup_t *me, cmt_rollup_advance_t *advance);

int cmt_rollup_read_inspect_state(cmt_rollup_t *me, cmt_rollup_inspect_t *inspect);

int cmt_rollup_finish(cmt_rollup_t *me, cmt_rollup_finish_t *finish);

int cmt_gio_request(cmt_rollup_t *me, cmt_gio_t *req);

int cmt_rollup_load_merkle(cmt_rollup_t *me, const char *path);

int cmt_rollup_save_merkle(cmt_rollup_t *me, const char *path);

void cmt_rollup_reset_merkle(cmt_rollup_t *me);
