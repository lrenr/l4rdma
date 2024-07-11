#pragma once

#include <l4/re/env>
#include "mem.h"
#include "cmd.h"

namespace Driver {

cu32 INIT_TIMEOUT_MS = 5000;

#pragma pack(4)
struct Init_Seg {
    reg32 fw_rev;
    reg32 cmdif_rev_fw_sub;
    reg32 rsvd0[2];
    reg32 cmdq_addr_msb;
    reg32 cmdq_addr_lsb_info;
    reg32 dbv;
    reg32 rsvd1[120];
    reg32 initializing;
};
#pragma pack()

void debug_cmd(volatile CMD::CQ &cq, l4_uint32_t slot);

void init_wait(reg32* initializing);

l4_uint32_t get_issi_support(volatile CMD::CQ &cq, l4_uint32_t slot, MEM::DMA_MEM* omb_mem);

l4_int32_t get_number_of_pages(volatile CMD::CQ &cq, l4_uint32_t slot);

void set_driver_version(volatile CMD::CQ &cq, reg32* dbv, MEM::DMA_MEM* imb_mem);

l4_uint32_t configure_issi(volatile CMD::CQ &cq, reg32* dbv, MEM::DMA_MEM* omb_mem);

bool configure_hca_cap(volatile CMD::CQ &cq, reg32* dbv, MEM::DMA_MEM* imb_mem, MEM::DMA_MEM* omb_mem);

void provide_pages(volatile CMD::CQ &cq, reg32* dbv, MEM::DMA_MEM* imb_mem, MEM::DMA_MEM* init_page_mem, l4_uint32_t init_page_count);

l4_int32_t provide_boot_pages(volatile CMD::CQ &cq, dma& dma_cap, reg32* dbv, MEM::DMA_MEM* imb_mem, MEM::HCA_DMA_MEM& hca_dma_mem);

l4_int32_t provide_init_pages(volatile CMD::CQ &cq, dma& dma_cap, reg32* dbv, MEM::DMA_MEM* imb_mem, MEM::HCA_DMA_MEM& hca_dma_mem);

l4_uint32_t reclaim_pages(volatile CMD::CQ &cq, reg32* dbv, MEM::DMA_MEM* omb_mem);

void init_hca(CMD::CQ& cq, dma& dma_cap, Init_Seg* init_seg, MEM::DMA_MEM* cq_mem, MEM::DMA_MEM* imb_mem, MEM::DMA_MEM* omb_mem, MEM::HCA_DMA_MEM& hca_dma_mem);

void teardown_hca(CMD::CQ& cq, Init_Seg* init_seg, MEM::DMA_MEM* omb_mem);

}
