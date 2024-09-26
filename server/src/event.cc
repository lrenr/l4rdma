#include <bitset>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include "event.h"
#include "device.h"

using namespace Device;
using namespace CMD;

void Event::init_eq(Q::Queue_Obj* eq) {
    /* set ownership bit to 1 */
    for (l4_size_t i = 0; i < eq->data.size - 1; i++) {
        iowrite32be(&((EQE*)eq->data.start)[i].ctrl, 0x1);
    }
}

bool Event::eqe_owned_by_hw(Driver::MLX5_Context& ctx, l4_uint32_t eq_number) {
    Q::Queue_Obj* eq = ctx.event_queue_pool.data.index[eq_number];

    l4_uint32_t ownership = ioread32be(&((EQE*)eq->data.start)[eq->data.head].ctrl);
    ownership &= EQC_OWNERSHIP_MASK;
    return ownership;
}

void Event::read_eqe(Driver::MLX5_Context& ctx, l4_uint32_t eq_number, l4_uint32_t* payload) {
    Q::Queue_Obj* eq = ctx.event_queue_pool.data.index[eq_number];

    EQE* eqe = &((EQE*)eq->data.start)[Q::enqueue(eq->data)];
    for (int i = 0; i < 7; i++)
        payload[i] = ioread32be(&eqe->data[i]);
}

l4_uint32_t Event::create_eq(Driver::MLX5_Context& ctx, l4_size_t size, l4_uint32_t type, l4_uint32_t irq_num) {
    l4_uint32_t slot;
    Q::Queue_Obj* eq = Q::alloc_queue(&ctx.event_queue_pool, size);
    EQ_CTX* eq_ctx = (EQ_CTX*)eq->data.q_ctx;
    eq_ctx->irq_num = irq_num;
    eq_ctx->type = type;

    cu32 page_count = eq->data.size % PAGE_EQE_COUNT ? (eq->data.size / PAGE_EQE_COUNT) + 1 : eq->data.size / PAGE_EQE_COUNT;
    if (page_count > (128 - EQI_SIZE) / 2) throw std::runtime_error("Event Queue too large!");

    EQI eqi = {};
    l4_uint32_t log_size = (l4_uint32_t)log2(eq->data.size);
    eqi.eqc.log_eq_size_and_uar = log_size << EQC_LOG_EQ_SIZE_OFFSET & EQC_LOG_EQ_SIZE_MASK;
    eqi.eqc.log_eq_size_and_uar |= eq->data.uarp->data.uar.index << EQC_UAR_OFFSET & EQC_UAR_MASK;
    eqi.eqc.intr = eq_ctx->irq_num << EQC_INTR_OFFSET & EQC_INTR_MASK;
    std::bitset<64> eq_type{0};
    if (eq_ctx->type != 0) eq_type[eq_ctx->type] = true;
    eqi.type[0] = (l4_uint32_t)(eq_type.to_ullong() >> 32);
    eqi.type[1] = (l4_uint32_t)eq_type.to_ullong();
    
    l4_uint32_t payload[128];
    l4_uint32_t payload_length = EQI_SIZE + (page_count * 2);
    printf("EQI_SIZE: %u | page_count: %u | payload_length: %u | eq_type: %llu\n", EQI_SIZE, page_count, payload_length, eq_type.to_ullong());

    for (l4_uint32_t i = 0; i < EQI_SIZE; i++)
        payload[i] = ((l4_uint32_t*)&eqi)[i];

    for (l4_uint32_t i = 0; i < page_count; i++) {
        payload[EQI_SIZE + i] = eq->data.pas_list[i];
    }
    init_eq(eq);

    /* CREATE_EQ */
    slot = create_cqe(ctx, CREATE_EQ, 0x0, payload, payload_length, CREATE_EQ_OUTPUT_LENGTH);
    ring_doorbell(ctx.dbv, &slot, 1);
    validate_cqe(ctx.cq, &slot, 1);

    l4_uint32_t output[CREATE_EQ_OUTPUT_LENGTH];
    get_cmd_output(ctx, slot, output, CREATE_EQ_OUTPUT_LENGTH);

    l4_uint32_t eq_number = output[0];
    printf("eq_number: %d\n", eq_number);
    eq->data.id = eq_number;

    Q::index_queue(&ctx.event_queue_pool, eq);
    printf("used: %d\n", eq->used);
    return eq_number;
}

l4_uint32_t Event::get_eq_state(Driver::MLX5_Context& ctx, l4_uint32_t eq_number) {
    l4_uint32_t slot;
    Q::Queue_Obj* eq = ctx.event_queue_pool.data.index[eq_number];

    cu32 page_count = eq->data.size % PAGE_EQE_COUNT ? (eq->data.size / PAGE_EQE_COUNT) + 1 : eq->data.size / PAGE_EQE_COUNT;
    l4_uint32_t payload[2] = {eq->data.id, 0};
    l4_uint32_t output_length = EQI_SIZE + (page_count * 2);

    /* QUERY_EQ */
    slot = create_cqe(ctx, QUERY_EQ, 0x0, payload, 2, output_length);
    ring_doorbell(ctx.dbv, &slot, 1);
    validate_cqe(ctx.cq, &slot, 1);

    l4_uint32_t output[128];
    get_cmd_output(ctx, slot, output, output_length);

    EQI* eqi = (EQI*)output;
    if ((eqi->eqc.status & EQC_STATUS_MASK) >> EQC_STATUS_OFFSET)
        throw std::runtime_error("EQC Error!");

    return (eqi->eqc.status & EQC_STATE_MASK) >> EQC_STATE_OFFSET;
}

void Event::destroy_eq(Driver::MLX5_Context& ctx, l4_uint32_t eq_number) {
    l4_uint32_t slot;
    Q::Queue_Obj* eq = ctx.event_queue_pool.data.index[eq_number];
    l4_uint32_t payload[2] = {eq->data.id, 0};

    /* DESTROY_EQ */
    slot = create_cqe(ctx, DESTROY_EQ, 0x0, payload, 2, DESTROY_EQ_OUTPUT_LENGTH);
    ring_doorbell(ctx.dbv, &slot, 1);
    validate_cqe(ctx.cq, &slot, 1);

    printf("destroyed eq: %d\n", eq_number);
}