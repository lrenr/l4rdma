#pragma once

#include <l4/re/env>
#include "mem.h"
#include "queue.h"
#include "device.h"
#include "driver.h"

namespace Event {

cu32 EQC_STATUS_MASK        = 0xf0000000;
cu32 EQC_STATE_MASK         = 0x00000f00;
cu32 EQC_UAR_MASK           = 0x00ffffff;
cu32 EQC_LOG_EQ_SIZE_MASK   = 0x1f000000;
cu32 EQC_INTR_MASK          = 0x000000ff;
cu32 EQC_LOG_PAGE_SIZE_MASK = 0x1f000000;
cu32 EQC_OWNERSHIP_MASK     = 0x00000001;

cu32 EQC_STATUS_OFFSET          = 28;
cu32 EQC_STATE_OFFSET           = 8;
cu32 EQC_UAR_OFFSET             = 0;
cu32 EQC_LOG_EQ_SIZE_OFFSET     = 24;
cu32 EQC_INTR_OFFSET            = 0;
cu32 EQC_LOG_PAGE_SIZE_OFFSET   = 24;

cu32 PAGE_EQE_COUNT = HCA_PAGE_SIZE / 64;

#pragma pack(4)
struct EQE {
    reg32 type = 0;
    reg32 rsvd[7];
    reg32 data[7];
    reg32 ctrl = 0;
};

struct EQC {
    reg32 status = 0;
    reg32 rsvd0[1];
    reg32 page_offset = 0;
    reg32 log_eq_size_and_uar = 0;
    reg32 rsvd1[1];
    reg32 intr = 0;
    reg32 log_page_size = 0;
    reg32 rsvd2[3];
    reg32 consumer_counter = 0;
    reg32 producer_counter = 0;
    reg32 rsvd3[4];
};

/* Event Queue Input Structure */
struct EQI {
    reg32 rsvd0[2];
    EQC eqc;
    reg32 rsvd1[2];
    reg32 type[2];
    reg32 rsvd2[44];
};
#pragma pack()

cu32 EQI_SIZE = sizeof(EQI) / 4;

struct EQ_CTX {
    l4_uint32_t irq_num;
    l4_uint32_t type;
};

enum EVENT_TYPE {
    EVENT_TYPE_COMPLETION_EVENT                = 0x0,
    EVENT_TYPE_LAST_WQE_REACHED                = 0x13,
    EVENT_TYPE_CQ_ERROR                        = 0x4,
    EVENT_TYPE_INTERNAL_ERROR                  = 0x8,
    EVENT_TYPE_PORT_STATE_CHANGE               = 0x9,
    EVENT_TYPE_TEMP_WARNING_EVENT              = 0x17,
    EVENT_TYPE_COMMAND_INTERFACE_COMPLETION    = 0xa,
    EVENT_TYPE_PAGE_REQUEST                    = 0xb,
};

enum EQ_STATUS {
    EQ_STATUS_OK                 = 0x0,
    EQ_STATUS_EQ_WRITE_FAILURE   = 0xa,
};

inline void arm_eq(Driver::MLX5_Context& ctx, l4_uint32_t eq_number) {
    Q::Queue_Obj* eq = ctx.event_queue_pool.data.index[eq_number];

    Device::iowrite32be(&eq->data.uarp->data.uar.addr->eqn_arm_and_update_ci, eq->data.id << UAR::UAR_EQN_OFFSET & UAR::UAR_EQN_MASK);
}

void init_eq(Q::Queue_Obj* eq);

bool eqe_owned_by_hw(Driver::MLX5_Context& ctx, l4_uint32_t eq_number);

void read_eqe(Driver::MLX5_Context& ctx, l4_uint32_t eq_number, l4_uint32_t* payload);

l4_uint32_t create_eq(Driver::MLX5_Context& ctx, l4_size_t size, l4_uint32_t type, l4_uint32_t irq_num);

l4_uint32_t get_eq_state(Driver::MLX5_Context& ctx, l4_uint32_t eq_number);

void destroy_eq(Driver::MLX5_Context& ctx, l4_uint32_t eq_number);

}