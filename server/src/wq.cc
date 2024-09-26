#include <cmath>
#include "wq.h"

l4_uint32_t WQ::create_wqc(Driver::MLX5_Context& ctx, l4_size_t size, l4_uint32_t wq_number) {
    Q::Queue_Obj* wq = Q::alloc_queue(&ctx.work_queue_pool, size);
    wq->data.id = wq_number;
    Q::index_queue(&ctx.work_queue_pool, wq);

    MEM::MEM_Page* mp = MEM::alloc_page(&ctx.mem_page_pool);
    l4_uint64_t phys = mp->data.phys;

    WQ_CTX* wq_ctx = (WQ_CTX*)wq->data.q_ctx;
    wq_ctx->wqc.type = 0;
    wq_ctx->wqc.page_offset_and_lwm = 0;
    wq_ctx->wqc.pd = Driver::alloc_pd(ctx);
    wq_ctx->wqc.uar = wq->data.uarp->data.uar.index;
    wq_ctx->wqc.dbr[0] = (l4_uint32_t)(phys >> 32);
    wq_ctx->wqc.dbr[1] = (l4_uint32_t)phys;
    wq_ctx->wqc.hw_counter = 0;
    wq_ctx->wqc.sw_counter = 0;
    wq_ctx->wqc.log_wq_sizes = (l4_uint8_t)std::log2(size * HCA_PAGE_SIZE) << WQE_WQ_SIZE_OFFSET & WQE_WQ_SIZE_MASK;

    return wq_number;
}

void WQ::destroy_wqc(Driver::MLX5_Context& ctx, l4_uint32_t wq_number) {
    Q::Queue_Obj* wq = Q::get_queue(&ctx.work_queue_pool, wq_number);
    WQ_CTX* wq_ctx = (WQ_CTX*)wq->data.q_ctx;
    Driver::dealloc_pd(ctx, wq_ctx->wqc.pd);

    //TODO clean up
}