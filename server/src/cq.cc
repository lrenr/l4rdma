#include <cmath>
#include "event.h"
#include "cq.h"

l4_uint32_t CQ::create_cq(Driver::MLX5_Context& ctx, l4_size_t size) {
    using namespace CMD;

    l4_uint32_t slot;

    MEM::MEM_Page* mp = MEM::alloc_page(&ctx.mem_page_pool);
    l4_uint64_t phys = mp->data.phys;

    l4_uint32_t eq_number = Event::create_eq(ctx, 4, Event::EVENT_TYPE_COMPLETION_EVENT, 0);

    Q::Queue_Obj* cq = Q::alloc_queue(&ctx.compl_queue_pool, size);

    CQC cqc;
    cqc.ctrl = 0;
    cqc.rsvd0[0] = 0;
    cqc.page_offset = 0;
    cqc.log_cq_size_and_uar_page = (l4_uint8_t)std::log2(size) << CQC_SIZE_OFFSET & CQC_SIZE_MASK;
    cqc.log_cq_size_and_uar_page |= cq->data.uarp->data.uar.index << CQC_UAR_OFFSET & CQC_UAR_MASK;
    cqc.cq_period_and_cq_max_count = 0;
    cqc.eqn = eq_number;
    cqc.log_page_size = 0;
    cqc.rsvd1[0] = 0;
    cqc.last_notified_index = 0;
    cqc.last_solicit_index = 0;
    cqc.consumer_counter = 0;
    cqc.producer_counter = 0;
    cqc.rsvd2[0] = 0;
    cqc.rsvd2[1] = 0;
    cqc.dbr[0] = (l4_uint32_t)(phys >> 32);
    cqc.dbr[1] = (l4_uint32_t)phys;

    cu32 page_count = size % PAGE_CQE_COUNT ? (size / PAGE_CQE_COUNT) + 1 : size / PAGE_CQE_COUNT;

    l4_uint32_t payload[128];
    l4_uint32_t payload_length = 2 + CQC_SIZE + 24 + (page_count * 2);
    printf("CQC_SIZE: %u | page_count: %u | payload_length: %u\n", CQC_SIZE, page_count, payload_length);

    payload[0] = 0;
    payload[1] = 0;

    for (l4_uint32_t i = 0; i < CQC_SIZE; i++)
        payload[2 + i] = ((l4_uint32_t*)&cqc)[i];

    for (l4_uint32_t i = 0; i < page_count; i++) {
        payload[2 + CQC_SIZE + 24 + i] = cq->data.pas_list[i];
    }

    /* CREATE_CQ */
    slot = create_cqe(ctx, CREATE_CQ, 0x0, payload, payload_length, CREATE_CQ_OUTPUT_LENGTH);
    ring_doorbell(ctx.dbv, &slot, 1);
    validate_cqe(ctx.cq, &slot, 1);

    l4_uint32_t output[CREATE_CQ_OUTPUT_LENGTH];
    get_cmd_output(ctx, slot, output, CREATE_CQ_OUTPUT_LENGTH);

    l4_uint32_t cq_number = output[0];
    printf("cq_number: %d | c_eqn: %d\n", cq_number, eq_number);
    cq->data.id = cq_number;

    Q::index_queue(&ctx.compl_queue_pool, cq);
    return cq_number;
}