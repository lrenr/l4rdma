#pragma once

#include "mem.h"
#include "driver.h"

namespace CQ {

cu32 CQC_SIZE_MASK      = 0x1f000000;
cu32 CQC_UAR_MASK       = 0x00ffffff;

cu32 CQC_SIZE_OFFSET    = 24;
cu32 CQC_UAR_OFFSET     = 0;

cu32 PAGE_CQE_COUNT = HCA_PAGE_SIZE / 64;
cu32 CQC_SIZE = 16;

#pragma pack(4)
struct CQC {
    reg32 ctrl;
    reg32 rsvd0[1];
    reg32 page_offset;
    reg32 log_cq_size_and_uar_page;
    reg32 cq_period_and_cq_max_count;
    reg32 eqn;
    reg32 log_page_size;
    reg32 rsvd1[1];
    reg32 last_notified_index;
    reg32 last_solicit_index;
    reg32 consumer_counter;
    reg32 producer_counter;
    reg32 rsvd2[2];
    reg32 dbr[2];
};
#pragma pack()

struct CQ_CTX {
    l4_uint32_t eq_number;
};

l4_uint32_t create_cq(Driver::MLX5_Context& ctx, l4_size_t size);

}