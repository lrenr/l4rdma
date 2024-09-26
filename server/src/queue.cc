#include "queue.h"

l4_uint32_t Q::enqueue(Queue& q) {
    l4_uint32_t slot = q.head;
    if (++q.head >= q.size)
        q.head = 0;
    return slot;
}

void Q::free_block(Queue_Obj_Pool* qop, Queue_Obj_Block* qob) {
    remove_block_from_pool(qop, qob);

    for (l4_uint64_t i = 0; i < qop->block_size; i++) {
        Queue_Obj* qo = &qob->start[i];
        free(qo->data.q_ctx);
    }

    destroy_block(qob);
}

Q::Queue_Obj* Q::alloc_queue(Queue_Obj_Pool* qop, l4_size_t size) {
    Queue_Obj* qo = PA::alloc_page<QOP_PD, QOP_BD, QOP_ED>(qop);
    qo->data.uarp = UAR::alloc_page(qop->data.uar_page_pool);
    qo->data.size = size;
    printf("uar: %d\n", qo->data.uarp->data.uar.index);

    cu32 page_count = size % qop->data.page_entry_count ? (size / qop->data.page_entry_count) + 1 : size / qop->data.page_entry_count;
    qo->data.pas_list = (l4_uint32_t*)malloc(sizeof(l4_uint32_t) * page_count);

    MEM::alloc_dma_mem(*qop->data.dma_cap, HCA_PAGE_SIZE * page_count, &qo->data.dma_mem);
    l4_uint64_t phys;
    l4_uint32_t pas_offset;
    for (l4_uint32_t i = 0; i < page_count; i++) {
        phys = qo->data.dma_mem.phys + (i * HCA_PAGE_SIZE);
        pas_offset = (i * 2);
        qo->data.pas_list[pas_offset] = (l4_uint32_t)(phys >> 32);
        qo->data.pas_list[++pas_offset] = (l4_uint32_t)phys;
    }

    qo->data.start = qo->data.dma_mem.virt;
    qo->data.head = 0;
    return qo;
}

void Q::free_queue(Queue_Obj_Pool* qop, l4_uint64_t id) {
    auto n = qop->data.index.extract(id);
    Queue_Obj* qo = n.mapped();

    cu32 page_count = qo->data.size % qop->data.page_entry_count ? (qo->data.size / qop->data.page_entry_count) + 1 : qo->data.size / qop->data.page_entry_count;

    UAR::free_page(qop->data.uar_page_pool, qo->data.uarp);
    free(qo->data.pas_list);
    free_dma_mem(*qop->data.dma_cap, HCA_PAGE_SIZE * page_count, &qo->data.dma_mem);

    PA::free_page<QOP_PD, QOP_BD, QOP_ED>(qop, qo);
}