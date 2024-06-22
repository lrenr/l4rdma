#pragma once

#include <l4/re/env>
#include "mem.h"
#include "cmd.h"

namespace HCA {

#pragma pack(4)
struct Init_Seg {
    reg32 fw_rev;
    reg32 cmdif_rev_fw_sub;
    reg32 padding0[2];
    reg32 cmdq_addr_msb;
    reg32 cmdq_addr_lsb_info;
    reg32 dbv;
    reg32 padding1[120];
    reg32 initializing;
};
#pragma pack()

void debug_cmd(volatile CMD::CQ &cq, l4_uint32_t slot);

void init_wait(reg32* initializing);

l4_uint32_t get_issi_support(MEM::DMA_MEM &mailbox);

l4_uint32_t get_number_of_pages(volatile CMD::CQ &cq, l4_uint32_t slot);

void provide_pages(volatile CMD::CQ &cq, reg32* dbv, MEM::DMA_MEM& imb_mem, MEM::DMA_MEM* init_page_mem, l4_uint32_t init_page_count);

void init_hca(CMD::CQ& cq, dma& dma_cap, MEM::HCA_PAGE_MEM& hca_page_mem, Init_Seg* init_seg, MEM::DMA_MEM& cq_mem, MEM::DMA_MEM& imb_mem, MEM::DMA_MEM& omb_mem);

void teardown_hca(CMD::CQ& cq, HCA::Init_Seg* init_seg);

}