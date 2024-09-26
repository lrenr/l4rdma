#pragma once

#include "mem.h"
#include "driver.h"

namespace WQ {

cu32 WQE_TYPE_MASK              = 0xf0000000;
cu32 WQE_PAGE_OFFSET_MASK       = 0x001f0000;
cu32 WQE_WQ_STRIDE_MASK         = 0x000f0000;
cu32 WQE_WQ_PAGE_SIZE_MASK      = 0x00001f00;
cu32 WQE_WQ_SIZE_MASK           = 0x0000001f;

cu32 WQE_TYPE_OFFSET            = 28;
cu32 WQE_PAGE_OFFSET_OFFSET     = 16;
cu32 WQE_WQ_STRIDE_OFFSET       = 16;
cu32 WQE_WQ_PAGE_SIZE_OFFSET    = 8;
cu32 WQE_WQ_SIZE_OFFSET         = 0;

cu32 RMPC_STATE_MASK            = 0x00f00000;
cu32 RMPC_STATE_OFFSET          = 20;

#pragma pack(4)
struct WQC {
    reg32 type;
    reg32 page_offset_and_lwm;
    reg32 pd;
    reg32 uar;
    reg32 dbr[2];
    reg32 hw_counter;
    reg32 sw_counter;
    reg32 log_wq_sizes;
    reg32 rsvd[39];
};

/*struct RMPC {
    reg32 state;
    reg32 cyclic_rev;
    reg32 rsvd[10];
    WQC wqc;
};*/

struct RQC {
    reg32 ctrl;
    reg32 user_index;
    reg32 cqn;
    reg32 rsvd0[1];
    reg32 rmpn;
    reg32 rsvd1[7];
};

struct SQC {
    reg32 ctrl;
    reg32 user_index;
    reg32 cqn;
    reg32 rsvd0[5];
    reg32 tis_lst_size;
    reg32 rsvd1[2];
    reg32 tis_num;
};
#pragma pack()

struct WQ_CTX {
    WQC wqc;
};

l4_uint32_t create_wqc(Driver::MLX5_Context& ctx, l4_size_t size, l4_uint32_t wq_number);

void destroy_wqc(Driver::MLX5_Context& ctx, l4_uint32_t wq_number);

}