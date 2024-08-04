#pragma once

#include <l4/re/env>
#include "mem.h"
#include "cmd.h"
#include "event.h"
#include "uar.h"

namespace Driver {

cu32 INIT_TIMEOUT_MS = 5000;
cu32 IMB_MAX_PAGE_PAYLOAD = 2 + CMD::IMB_MAX_DATA;

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

struct PRH_OPT {
    MEM::Queue<Event::EQE>* eq;
    bool active;
};

void debug_cmd(CMD::CMD_Args& cmd_args, l4_uint32_t slot);

void init_wait(reg32* initializing);

l4_uint32_t get_issi_support(CMD::CMD_Args& cmd_args, l4_uint32_t slot);

l4_int32_t get_number_of_pages(CMD::CMD_Args& cmd_args, l4_uint32_t slot);

void set_driver_version(CMD::CMD_Args& cmd_args);

l4_uint32_t configure_issi(CMD::CMD_Args& cmd_args);

bool configure_hca_cap(CMD::CMD_Args& cmd_args);

void provide_pages(CMD::CMD_Args& cmd_args, MEM::DMA_MEM* init_page_mem, l4_uint32_t init_page_count);

l4_int32_t provide_boot_pages(CMD::CMD_Args& cmd_args, dma& dma_cap, MEM::HCA_DMA_MEM& hca_dma_mem);

l4_int32_t provide_init_pages(CMD::CMD_Args& cmd_args, dma& dma_cap, MEM::HCA_DMA_MEM& hca_dma_mem);

l4_uint32_t reclaim_pages(CMD::CMD_Args& cmd_args);

void init_hca(CMD::CMD_Args& cmd_args, dma& dma_cap, Init_Seg* init_seg, MEM::DMA_MEM* cq_mem, MEM::HCA_DMA_MEM& hca_dma_mem);

void teardown_hca(CMD::CMD_Args& cmd_args);

UAR::UAR alloc_uar(CMD::CMD_Args& cmd_args, l4_uint8_t* bar0);

void* page_request_handler(void* arg);

void setup_event_queue(CMD::CMD_Args& cmd_args, l4_uint64_t icu_src, reg32* msix_table, L4::Cap<L4::Icu>& icu, MEM::HCA_DMA_MEM& hca_dma_mem, dma& dma_cap, UAR::UAR uar);

}
