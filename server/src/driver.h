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

struct HCA_CAP {};

struct MLX5_Context {
    CMD::CMD_Args cmd_args;
    HCA_CAP hca_cap;
    UAR::UAR_Page_Pool uar_page_pool;
    MEM::MEM_Page_Pool mem_page_pool;
};

struct PRH_OPT {
    MLX5_Context* ctx;
    MEM::Queue<Event::EQE>* eq;
    bool active;
};

void debug_cmd(MLX5_Context& ctx, l4_uint32_t slot);

void init_wait(reg32* initializing);

l4_uint32_t get_issi_support(MLX5_Context& ctx, l4_uint32_t slot);

l4_int32_t get_number_of_pages(MLX5_Context& ctx, l4_uint32_t slot);

void set_driver_version(MLX5_Context& ctx);

l4_uint32_t configure_issi(MLX5_Context& ctx);

bool configure_hca_cap(MLX5_Context& ctx);

void provide_pages(MLX5_Context& ctx, l4_uint32_t init_page_count);

l4_int32_t provide_boot_pages(MLX5_Context& ctx);

l4_int32_t provide_init_pages(MLX5_Context& ctx);

l4_uint32_t reclaim_pages(MLX5_Context& ctx, l4_int32_t page_count);

l4_uint32_t reclaim_all_pages(MLX5_Context& ctx);

void init_hca(MLX5_Context& ctx, Init_Seg* init_seg);

void teardown_hca(MLX5_Context& ctx);

UAR::UAR alloc_uar(MLX5_Context& ctx, l4_uint8_t* bar0);

void* page_request_handler(void* arg);

void setup_event_queue(MLX5_Context& ctx, l4_uint64_t icu_src, reg32* msix_table, L4::Cap<L4::Icu>& icu, dma& dma_cap, UAR::UAR uar);

}
