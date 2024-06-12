#pragma once

#include <l4/re/env>
#include "hca.h"

namespace cmd {

typedef l4_uint32_t reg32;
typedef l4_uint64_t reg64;

const reg64 CQE_MAILBOX_MASK    = ~0x1ff;
const reg32 CQE_TYPE_MASK       = 0xff000000;
const reg32 CQE_TOKEN_MASK      = 0xff000000;
const reg32 CQE_SIGNATURE_MASK  = 0x00ff0000;
const reg32 CQE_STATUS_MASK     = 0x000000fe;
const reg32 CQE_OWNERSHIP_MASK  = 0x00000001;
const reg32 CID_OPCODE_MASK     = 0xffff0000;
const reg32 CID_OP_MOD_MASK     = 0x0000ffff;
const reg32 COD_STATUS_MASK     = 0xff000000;

const unsigned int CQE_TYPE_OFFSET        = 24;
const unsigned int CQE_TOKEN_OFFSET       = 24;
const unsigned int CQE_SIGNATURE_OFFSET   = 16;
const unsigned int CQE_STATUS_OFFSET      = 1;
const unsigned int CID_OPCODE_OFFSET      = 16;
const unsigned int COD_STATUS_OFFSET      = 24;

#pragma pack(4)
// Command Input Data
struct CID {
    reg32 opcode = 0;
    reg32 op_mod = 0;
    reg32 opt[2] = {0, 0};
};

// Command Output Data
struct COD {
    reg32 status = 0;
    reg32 syndrome = 0;
    reg32 output[2] = {0, 0};
};

// Command Queue Entry
struct CQE {
    reg32 type = 0;
    reg32 input_lenght = 0;
    reg64 input_mailbox = 0;
    CID cid;
    COD cod;
    reg64 output_mailbox = 0;
    reg32 output_lenght = 0;
    reg32 ctrl = 0;
};
#pragma pack()

// Command Queue Ring Buffer
struct CQ {
    CQE* start;
    l4_size_t size;
    l4_uint32_t head;
};

void poll_ownership(CQE &cqe);

void check_cqe_status(CQE &cqe);

void check_cod_status(COD &cod);

CQE create_cqe(hca::OPCODE opcode, reg32 op_mod);

l4_uint32_t enqueue_cqe(CQE &cqe, CQ &cq);

void validate_cqe(CQE &cqe);

}