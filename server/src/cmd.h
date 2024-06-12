#pragma once

#include <l4/re/env>

l4_uint64_t CQE_MAILBOX_MASK    = ~0x1ff;
l4_uint32_t CQE_TYPE_MASK       = 0xff000000;
l4_uint32_t CQE_TOKEN_MASK      = 0xff000000;
l4_uint32_t CQE_SIGNATURE_MASK  = 0x00ff0000;
l4_uint32_t CQE_STATUS_MASK     = 0x000000fe;
l4_uint32_t CQE_OWNERSHIP_MASK  = 0x00000001;
l4_uint32_t CID_OPCODE_MASK     = 0xffff0000;
l4_uint32_t CID_OP_MOD_MASK     = 0x0000ffff;
l4_uint32_t COD_STATUS_MASK     = 0xff000000;

unsigned int CQE_TYPE_OFFSET        = 24;
unsigned int CQE_TOKEN_OFFSET       = 24;
unsigned int CQE_SIGNATURE_OFFSET   = 16;
unsigned int CQE_STATUS_OFFSET      = 1;
unsigned int CID_OPCODE_OFFSET      = 16;
unsigned int COD_STATUS_OFFSET      = 24;

#pragma pack (4)
struct CID {
    l4_uint32_t opcode;
    l4_uint32_t op_mod;
    l4_uint32_t cmd[2];
};

struct COD {
    l4_uint32_t status;
    l4_uint32_t syndrome;
    l4_uint32_t output[2];
};

struct CQE {
    l4_uint32_t type;
    l4_uint32_t input_lenght;
    l4_uint64_t input_mailbox;
    CID input_inline;
    COD output_inline;
    l4_uint64_t output_mailbox;
    l4_uint32_t output_lenght;
    l4_uint32_t ctrl;
};
#pragma pack (0)

CQE blank_cqe() {
    CQE blank;
    blank.type = CQE_TYPE_MASK & (0x7U << CQE_TYPE_OFFSET);
    blank.input_lenght = 0;
    blank.input_mailbox = 0;
    blank.output_mailbox = 0;
    blank.output_lenght = 0;
    blank.ctrl = 0;
    return blank;
}

bool poll_ownership(CQE &cqe) {
    while (!(cqe.ctrl & CQE_OWNERSHIP_MASK));
    return true;
}