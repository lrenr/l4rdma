#include <bitset>
#include <cmath>
#include <stdexcept>
#include "event.h"
#include "device.h"

using namespace Device;
using namespace CMD;

void Event::init_eq(MEM::Queue<EQE>& eq) {
    /* set ownership bit to 1 */
    for (l4_size_t i = 0; i < eq.size - 1; i++) {
        iowrite32be(&eq.start[i].ctrl, 0x1);
    }
}

bool Event::eqe_owned_by_hw(MEM::Queue<EQE>& eq) {
    l4_uint32_t ownership = ioread32be(&eq.start[eq.head].ctrl);
    ownership &= EQC_OWNERSHIP_MASK;
    return ownership;
}

void Event::read_eqe(MEM::Queue<EQE>& eq, l4_uint32_t* payload) {
    EQE* eqe = &eq.start[MEM::enqueue(eq)];
    for (int i = 0; i < 7; i++)
        payload[i] = ioread32be(&eqe->data[i]);
}

void Event::create_eq(CMD_Args& cmd_args, MEM::Queue<EQE>& eq, l4_uint32_t type, l4_uint32_t irq_num, l4_uint32_t uar, MEM::HCA_DMA_MEM& hca_dma_mem, dma& dma_cap) {
    l4_uint32_t slot;

    cu32 page_count = eq.size % PAGE_EQE_COUNT ? (eq.size / PAGE_EQE_COUNT) + 1 : eq.size / PAGE_EQE_COUNT;
    if (page_count > (128 - EQI_SIZE) / 2) throw std::runtime_error("Event Queue too large!");

    EQI eqi = {};
    l4_uint32_t log_size = (l4_uint32_t)log2(eq.size);
    eqi.eqc.log_eq_size_and_uar = log_size << EQC_LOG_EQ_SIZE_OFFSET & EQC_LOG_EQ_SIZE_MASK;
    eqi.eqc.log_eq_size_and_uar |= uar << EQC_UAR_OFFSET & EQC_UAR_MASK;
    eqi.eqc.intr = irq_num << EQC_INTR_OFFSET & EQC_INTR_MASK;
    std::bitset<64> eq_type{0};
    eq_type[type] = true;
    eqi.type[0] = (l4_uint32_t)(eq_type.to_ullong() >> 32);
    eqi.type[1] = (l4_uint32_t)eq_type.to_ullong();
    
    l4_uint32_t payload[128];
    l4_uint32_t payload_length = EQI_SIZE + (page_count * 2);
    printf("EQI_SIZE: %u | page_count: %u | payload_length: %u | eq_type: %llu\n", EQI_SIZE, page_count, payload_length, eq_type.to_ullong());

    for (l4_uint32_t i = 0; i < EQI_SIZE; i++)
        payload[i] = ((l4_uint32_t*)&eqi)[i];

    MEM::DMA_MEM* eq_page_mem = MEM::alloc_dma_mem(dma_cap, HCA_PAGE_SIZE * page_count, &hca_dma_mem.dma_mem[hca_dma_mem.dma_mem_count]);
    hca_dma_mem.dma_mem_count++;
    l4_uint64_t phys;
    l4_uint32_t pas_offset;
    for (l4_uint32_t i = 0; i < page_count; i++) {
        phys = eq_page_mem->phys + (i * HCA_PAGE_SIZE);
        pas_offset = EQI_SIZE + (i * 2);
        payload[pas_offset] = (l4_uint32_t)(phys >> 32);
        payload[++pas_offset] = (l4_uint32_t)phys;
    }

    eq.start = (EQE*)eq_page_mem->virt;
    eq.head = 0;
    init_eq(eq);

    /* CREATE_EQ */
    slot = create_cqe(cmd_args, CREATE_EQ, 0x0, payload, payload_length, CREATE_EQ_OUTPUT_LENGTH);
    ring_doorbell(cmd_args.dbv, &slot, 1);
    validate_cqe(cmd_args.cq, &slot, 1);

    l4_uint32_t output[CREATE_EQ_OUTPUT_LENGTH];
    get_cmd_output(cmd_args, slot, output, CREATE_EQ_OUTPUT_LENGTH);

    l4_uint32_t eq_number = output[0];
    printf("eq_number: %d\n", eq_number);
    eq.id = eq_number;
}

l4_uint32_t Event::get_eq_state(CMD_Args& cmd_args, MEM::Queue<EQE>& eq) {
    l4_uint32_t slot;

    cu32 page_count = eq.size % PAGE_EQE_COUNT ? (eq.size / PAGE_EQE_COUNT) + 1 : eq.size / PAGE_EQE_COUNT;
    l4_uint32_t payload[2] = {eq.id, 0};
    l4_uint32_t output_length = EQI_SIZE + (page_count * 2);

    /* QUERY_EQ */
    slot = create_cqe(cmd_args, QUERY_EQ, 0x0, payload, 2, output_length);
    ring_doorbell(cmd_args.dbv, &slot, 1);
    validate_cqe(cmd_args.cq, &slot, 1);

    l4_uint32_t output[128];
    get_cmd_output(cmd_args, slot, output, output_length);

    EQI* eqi = (EQI*)output;
    if ((eqi->eqc.status & EQC_STATUS_MASK) >> EQC_STATUS_OFFSET)
        throw std::runtime_error("EQC Error!");

    return (eqi->eqc.status & EQC_STATE_MASK) >> EQC_STATE_OFFSET;
}

void Event::destroy_eq(CMD_Args& cmd_args, MEM::Queue<EQE>& eq) {
    l4_uint32_t slot;
    l4_uint32_t payload[2] = {eq.id, 0};

    /* DESTROY_EQ */
    slot = create_cqe(cmd_args, DESTROY_EQ, 0x0, payload, 2, DESTROY_EQ_OUTPUT_LENGTH);
    ring_doorbell(cmd_args.dbv, &slot, 1);
    validate_cqe(cmd_args.cq, &slot, 1);
}