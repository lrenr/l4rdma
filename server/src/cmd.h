#pragma once

#include <l4/re/env>
#include "mem.h"
#include "queue.h"

namespace CMD {

cu32 CQE_MAILBOX_MASK    = ~0x1ff;
cu32 CQE_TYPE_MASK       = 0xff000000;
cu32 CQE_TOKEN_MASK      = 0xff000000;
cu32 CQE_SIGNATURE_MASK  = 0x00ff0000;
cu32 CQE_STATUS_MASK     = 0x000000fe;
cu32 CQE_OWNERSHIP_MASK  = 0x00000001;
cu32 CID_OPCODE_MASK     = 0xffff0000;
cu32 CID_OP_MOD_MASK     = 0x0000ffff;
cu32 COD_STATUS_MASK     = 0xff000000;

cu32 CQE_TYPE_OFFSET        = 24;
cu32 CQE_TOKEN_OFFSET       = 24;
cu32 CQE_SIGNATURE_OFFSET   = 16;
cu32 CQE_STATUS_OFFSET      = 1;
cu32 CID_OPCODE_OFFSET      = 16;
cu32 COD_STATUS_OFFSET      = 24;

cu32 MAILBOX_ALIGN_SIZE = 0x1000;
cu32 MBB_MAX_COUNT = 50; // has to be >=42 for some reason
cu32 IMB_MAX_DATA = 128 * MBB_MAX_COUNT;
cu32 OMB_MAX_DATA = 128 * MBB_MAX_COUNT;
cu32 CMD_TIMEOUT_MS = 5000;

cu32 QUERY_HCA_CAP_OUTPUT_LENGTH        = 406;
cu32 INIT_HCA_OUTPUT_LENGTH             = 0;
cu32 TEARDOWN_HCA_OUTPUT_LENGTH         = 0;
cu32 ENABLE_HCA_OUTPUT_LENGTH           = 0;
cu32 DISABLE_HCA_OUTPUT_LENGTH          = 0;
cu32 QUERY_PAGES_OUTPUT_LENGTH          = 2;
cu32 SET_HCA_CAP_OUTPUT_LENGTH          = 2;
cu32 QUERY_ISSI_OUTPUT_LENGTH           = 26;
cu32 SET_ISSI_OUTPUT_LENGTH             = 0;
cu32 SET_DRIVER_VERSION_OUTPUT_LENGTH   = 0;
cu32 CREATE_EQ_OUTPUT_LENGTH            = 2;
cu32 DESTROY_EQ_OUTPUT_LENGTH           = 0;

#pragma pack(4)
/* Command Input Data */
struct CID {
    reg32 opcode = 0;
    reg32 op_mod = 0;
    reg32 data[2] = {0, 0};
};

/* Command Output Data */
struct COD {
    reg32 status = 0;
    reg32 syndrome = 0;
    reg32 output[2] = {0, 0};
};

/* Command Queue Entry */
struct CQE {
    reg32 type = 0;
    reg32 input_length = 0;
    reg32 input_mailbox_msb = 0;
    reg32 input_mailbox_lsb = 0;
    CID cid;
    COD cod;
    reg32 output_mailbox_msb = 0;
    reg32 output_mailbox_lsb = 0;
    reg32 output_length = 0;
    reg32 ctrl = 0;
};

/* Mailbox Block */
struct MBB {
    reg32 mailbox_data[128];
    reg32 rsvd[12];
    reg32 next_pointer_msb = 0;
    reg32 next_pointer_lsb = 0;
    reg32 block_number = 0;
    reg32 ctrl = 0;
};
#pragma pack()

enum OPCODE {
    QUERY_HCA_CAP               = 0x100,
    QUERY_ADAPTER               = 0x101,
    INIT_HCA                    = 0x102,
    TEARDOWN_HCA                = 0x103,
    ENABLE_HCA                  = 0x104,
    DISABLE_HCA                 = 0x105,
    QUERY_PAGES                 = 0x107,
    MANAGE_PAGES                = 0x108,
    SET_HCA_CAP                 = 0x109,
    QUERY_ISSI                  = 0x10a,
    SET_ISSI                    = 0x10b,
    SET_DRIVER_VERSION          = 0x10d,
    CREATE_MKEY                 = 0x200,
    QUERY_MKEY                  = 0x201,
    DESTROY_MKEY                = 0x202,
    QUERY_SPECIAL_CONTEXTS      = 0x203,
    CREATE_EQ                   = 0x301,
    DESTROY_EQ                  = 0x302,
    QUERY_EQ                    = 0x303,
    GEN_EQE                     = 0x304,
    CREATE_CQ                   = 0x400,
    DESTROY_CQ                  = 0x401,
    QUERY_CQ                    = 0x402,
    MODIFY_CQ                   = 0x403,
    QUERY_VPORT_STATE           = 0x750,
    MODIFY_VPORT_STATE          = 0x751,
    QUERY_NIC_VPORT_CONTEXT     = 0x754,
    MODIFY_NIC_VPORT_CONTEXT    = 0x755,
    QUERY_VPORT_COUNTER         = 0x770,
    ALLOC_PD                    = 0x800,
    DEALLOC_PD                  = 0x801,
    ALLOC_UAR                   = 0x802,
    DEALLOC_UAR                 = 0x803,
    CONFIG_INT_MODERATION       = 0x804,
    ACCESS_REG                  = 0x805,
    NOP                         = 0x80d,
    ALLOC_TRANSPORT_DOMAIN      = 0x816,
    DEALLOC_TRANSPORT_DOMAIN    = 0x817,
    SET_L2_TABLE_ENTRY          = 0x829,
    QUERY_L2_TABLE_ENTRY        = 0x82a,
    DELETE_L2_TABLE_ENTRY       = 0x82b,
    CREATE_TIR                  = 0x900,
    MODIFY_TIR                  = 0x901,
    DESTROY_TIR                 = 0x902,
    QUERY_TIR                   = 0x903,
    CREATE_SQ                   = 0x904,
    MODIFY_SQ                   = 0x905,
    DESTROY_SQ                  = 0x906,
    QUERY_SQ                    = 0x907,
    CREATE_RQ                   = 0x908,
    MODIFY_RQ                   = 0x909,
    DESTROY_RQ                  = 0x90a,
    QUERY_RQ                    = 0x90b,
    CREATE_RMP                  = 0x90c,
    MODIFY_RMP                  = 0x90d,
    DESTROY_RMP                 = 0x90e,
    QUERY_RMP                   = 0x90f,
    CREATE_TIS                  = 0x912,
    MODIFY_TIS                  = 0x913,
    DESTROY_TIS                 = 0x914,
    QUERY_TIS                   = 0x915,
    CREATE_RQT                  = 0x916,
    MODIFY_RQT                  = 0x917,
    DESTROY_RQT                 = 0x918,
    QUERY_RQT                   = 0x919,
    SET_FLOW_TABLE_ROOT         = 0x92f,
    CREATE_FLOW_TABLE           = 0x930,
    DESTROY_FLOW_TABLE          = 0x931,
    QUERY_FLOW_TABLE            = 0x932,
    CREATE_FLOW_GROUP           = 0x933,
    DESTROY_FLOW_GROUP          = 0x934,
    QUERY_FLOW_GROUP            = 0x935,
    SET_FLOW_TABLE_ENTRY        = 0x936,
    QUERY_FLOW_TABLE_ENTRY      = 0x937,
    DELETE_FLOW_TABLE_ENTRY     = 0x938,
    ALLOC_FLOW_COUNTER          = 0x939,
    DEALLOC_FLOW_COUNTER        = 0x93a,
    QUERY_FLOW_COUNTER          = 0x93b,
    MODIFY_FLOW_TABLE           = 0x93c,
};

enum CMD_STATUS {
	CMD_STATUS_OK                       = 0x0,
	CMD_STATUS_INT_ERR                  = 0x1,
	CMD_STATUS_BAD_OP_ERR               = 0x2,
	CMD_STATUS_BAD_PARAM_ERR            = 0x3,
	CMD_STATUS_BAD_SYS_STATE_ERR        = 0x4,
	CMD_STATUS_BAD_RES_ERR              = 0x5,
	CMD_STATUS_RES_BUSY                 = 0x6,
	CMD_STATUS_LIM_ERR                  = 0x8,
	CMD_STATUS_BAD_RES_STATE_ERR        = 0x9,
	CMD_STATUS_IX_ERR                   = 0xa,
	CMD_STATUS_NO_RES_ERR               = 0xf,
	CMD_STATUS_BAD_INP_LEN_ERR          = 0x50,
	CMD_STATUS_BAD_OUTP_LEN_ERR         = 0x51,
	CMD_STATUS_BAD_QP_STATE_ERR         = 0x10,
	CMD_STATUS_BAD_PKT_ERR              = 0x30,
	CMD_STATUS_BAD_SIZE_OUTS_CQES_ERR   = 0x40,
};

enum CQE_STATUS {
    CQE_STATUS_OK           = 0x0,
    CQE_STATUS_BAD_SIG      = 0x1,
    CQE_STATUS_BAD_TOKEN    = 0x2,
    CQE_STATUS_BAD_BNUM     = 0x3,
    CQE_STATUS_BAD_OUT_PTR  = 0x4,
    CQE_STATUS_BAD_IN_PTR   = 0x5,
    CQE_STATUS_INTERNAL_ERR = 0x6,
    CQE_STATUS_IN_LEN_ERR   = 0x7,
    CQE_STATUS_OUT_LEN_ERR  = 0x8,
    CQE_STATUS_PAD_NOT_ZERO = 0x9,
    CQE_STATUS_BAD_TYPE     = 0x10,
};

/* All the things needed to execute commands */
struct CMD_Args {
    Q::Queue cq;
    reg32* dbv;
    MEM::DMA_MEM imb_mem;
    MEM::DMA_MEM omb_mem;
};

void poll_ownership(volatile CQE* cqe);

void check_cqe_status(volatile CQE* cqe);

void check_cod_status(volatile COD* cod);

void prepare_mbb(volatile MBB* mailbox_block, l4_uint64_t phys_addr, l4_uint32_t block_number);

void tie_mail_together(MEM::DMA_MEM* mailbox, l4_uint32_t mailbox_counter);

void pack_mail(MEM::DMA_MEM* mailbox, l4_uint32_t* payload, l4_uint32_t length);

l4_uint32_t create_cqe(CMD::CMD_Args& cmd_args, OPCODE opcode, l4_uint32_t op_mod,
    l4_uint32_t* payload, l4_uint32_t payload_length, l4_uint32_t output_length);

void unpack_mail(MEM::DMA_MEM* mailbox, l4_uint32_t* payload, l4_uint32_t length);

void get_cmd_output(CMD::CMD_Args& cmd_args, l4_uint32_t slot, l4_uint32_t* output, l4_uint32_t output_length);

void ring_doorbell(reg32* dbv, l4_uint32_t* slots, int count);

void validate_cqe(Q::Queue& cq, l4_uint32_t* slots, int count);

}
