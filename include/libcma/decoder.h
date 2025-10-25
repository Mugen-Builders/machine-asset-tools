#ifndef CMA_DECODER_H
#define CMA_DECODER_H
#include <stdint.h>

#include <libcmt/abi.h>

enum cma_decoder_request_type_t {
  CMA_DECODER_REQUEST_TYPE_NONE,
  CMA_DECODER_REQUEST_TYPE_ETHER_DEPOSIT,
  CMA_DECODER_REQUEST_TYPE_ERC20_DEPOSIT,
  CMA_DECODER_REQUEST_TYPE_ERC721_DEPOSIT,
  CMA_DECODER_REQUEST_TYPE_ERC1155_SINGLE_DEPOSIT,
  CMA_DECODER_REQUEST_TYPE_ERC1155_BATCH_DEPOSIT,
  CMA_DECODER_REQUEST_TYPE_ETHER_WITHDRAWAL,
  CMA_DECODER_REQUEST_TYPE_ERC20_WITHDRAWAL,
  CMA_DECODER_REQUEST_TYPE_ERC721_WITHDRAWAL,
  CMA_DECODER_REQUEST_TYPE_ERC1155_SINGLE_WITHDRAWAL,
  CMA_DECODER_REQUEST_TYPE_ERC1155_BATCH_WITHDRAWAL,
  CMA_DECODER_REQUEST_TYPE_ETHER_TRANSFER,
  CMA_DECODER_REQUEST_TYPE_ERC20_TRANSFER,
  CMA_DECODER_REQUEST_TYPE_ERC721_TRANSFER,
  CMA_DECODER_REQUEST_TYPE_ERC1155_SINGLE_TRANSFER,
  CMA_DECODER_REQUEST_TYPE_ERC1155_BATCH_TRANSFER,
};

struct cma_decoder_ether_deposit_t { /* fields... */
};
struct cma_decoder_erc20_deposit_t { /* fields... */
};
struct cma_decoder_erc721_deposit_t { /* fields... */
};
struct cma_decoder_erc1155_single_deposit_t { /* fields... */
};
struct cma_decoder_erc1155_batch_deposit_t { /* fields... */
};

struct cma_decoder_ether_withdrawal_t { /* fields... */
};
struct cma_decoder_erc20_withdrawal_t { /* fields... */
};
struct cma_decoder_erc721_withdrawal_t { /* fields... */
};
struct cma_decoder_erc1155_single_withdrawal_t { /* fields... */
};
struct cma_decoder_erc1155_batch_withdrawal_t { /* fields... */
};

struct cma_decoder_ether_transfer_t { /* fields... */
};
struct cma_decoder_erc20_transfer_t { /* fields... */
};
struct cma_decoder_erc721_transfer_t { /* fields... */
};
struct cma_decoder_erc1155_single_transfer_t { /* fields... */
};
struct cma_decoder_erc1155_batch_transfer_t { /* fields... */
};

struct cma_decoder_request_t {
  enum cma_decoder_request_type_t type;
  union {
    struct cma_decoder_ether_deposit_t ether_deposit;
    struct cma_decoder_erc20_deposit_t erc20_deposit;
    struct cma_decoder_erc721_deposit_t erc721_deposit;
    struct cma_decoder_erc1155_single_deposit_t erc1155_single_deposit;
    struct cma_decoder_erc1155_batch_deposit_t erc1155_batch_deposit;
    struct cma_decoder_ether_withdrawal_t ether_withdrawal;
    struct cma_decoder_erc20_withdrawal_t erc20_withdrawal;
    struct cma_decoder_erc721_withdrawal_t erc721_withdrawal;
    struct cma_decoder_erc1155_single_withdrawal_t erc1155_single_withdrawal;
    struct cma_decoder_erc1155_batch_withdrawal_t erc1155_batch_withdrawal;
    struct cma_decoder_ether_transfer_t ether_transfer;
    struct cma_decoder_erc20_transfer_t erc20_transfer;
    struct cma_decoder_erc721_transfer_t erc721_transfer;
    struct cma_decoder_erc1155_single_transfer_t erc1155_single_transfer;
    struct cma_decoder_erc1155_batch_transfer_t erc1155_batch_transfer;
  } u;
};

typedef enum {
  CMA_DECODER_SUCCESS = 0,
  CMA_DECODER_ERROR_UNKNOWN
} cma_decoder_error_t;

cma_decoder_error_t cma_decoder_rollup(struct cmt_rollup_t *rollup,
                                       struct cma_decoder_request_t *request);

// decode ether deposit
// decode erc20 deposit
// decode erc721 deposit
// decode erc1155 single deposit
// decode erc1155 batch deposit

// decode ether withdrawal
// decode erc20 withdrawal
// decode erc721 withdrawal
// decode erc1155 single withdrawal
// decode erc1155 batch withdrawal

// decode ether transfer
// decode erc20 transfer
// decode erc721 transfer
// decode erc1155 single transfer
// decode erc1155 batch transfer

// encode ether withdrawal voucher
// encode erc20 withdrawal voucher
// encode erc721 withdrawal voucher
// encode erc1155 single withdrawal voucher
// encode erc1155 batch withdrawal voucher

#endif // CMA_DECODER_H
