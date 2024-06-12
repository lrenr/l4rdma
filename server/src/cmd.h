#pragma once

#include <l4/re/env>

namespace cmd {

const l4_uint64_t CQE_MAILBOX_MASK    = ~0x1ff;
const l4_uint32_t CQE_TYPE_MASK       = 0xff000000;
const l4_uint32_t CQE_TOKEN_MASK      = 0xff000000;
const l4_uint32_t CQE_SIGNATURE_MASK  = 0x00ff0000;
const l4_uint32_t CQE_STATUS_MASK     = 0x000000fe;
const l4_uint32_t CQE_OWNERSHIP_MASK  = 0x00000001;
const l4_uint32_t CID_OPCODE_MASK     = 0xffff0000;
const l4_uint32_t CID_OP_MOD_MASK     = 0x0000ffff;
const l4_uint32_t COD_STATUS_MASK     = 0xff000000;

const unsigned int CQE_TYPE_OFFSET        = 24;
const unsigned int CQE_TOKEN_OFFSET       = 24;
const unsigned int CQE_SIGNATURE_OFFSET   = 16;
const unsigned int CQE_STATUS_OFFSET      = 1;
const unsigned int CID_OPCODE_OFFSET      = 16;
const unsigned int COD_STATUS_OFFSET      = 24;

#pragma pack (4)
struct CID {
    l4_uint32_t opcode = 0;
    l4_uint32_t op_mod = 0;
    l4_uint32_t cmd[2] = {0, 0};
};

struct COD {
    l4_uint32_t status = 0;
    l4_uint32_t syndrome = 0;
    l4_uint32_t output[2] = {0, 0};
};

struct CQE {
    l4_uint32_t type = 0;
    l4_uint32_t input_lenght = 0;
    l4_uint64_t input_mailbox = 0;
    CID input_inline;
    COD output_inline;
    l4_uint64_t output_mailbox = 0;
    l4_uint32_t output_lenght = 0;
    l4_uint32_t ctrl = 0;
};
#pragma pack (0)

CQE create_cqe_exmail(CID cid);

bool poll_ownership(CQE &cqe);

void check_cqe_status(CQE &cqe);

void check_cod_status(COD &cod);

}