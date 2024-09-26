#pragma once

#include <l4/re/env>
#include <l4/re/dma_space>
#include <l4/re/dataspace>
#include <l4/re/util/shared_cap>
#include "mem.h"
#include "uar.h"

namespace Q {

/* generic HCA Queue */
struct Queue {
    void* start;
    l4_size_t size;
    l4_uint32_t head = 0;
    MEM::DMA_MEM dma_mem;
};

l4_uint32_t enqueue(Queue& q);

/* queue_pool definitions */
struct QOP_PD;

struct QOP_BD {};

struct QOP_ED : Q::Queue {
    l4_uint32_t id;
    UAR::UAR_Page* uarp;
    l4_uint32_t* pas_list;
    void* q_ctx;
};

typedef PA::Pool<QOP_PD, QOP_BD, QOP_ED> Queue_Obj_Pool;
typedef PA::Block<QOP_BD, QOP_ED> Queue_Obj_Block;
typedef PA::Element<QOP_BD, QOP_ED> Queue_Obj;

struct QOP_PD {
    l4_uint32_t page_entry_count;
    dma* dma_cap;
    UAR::UAR_Page_Pool* uar_page_pool;
    std::unordered_map<l4_uint64_t, Queue_Obj*> index;
};

template<typename Q_CTX>
void alloc_block(Queue_Obj_Pool* qop){
    Queue_Obj_Block* qob = create_block(qop);

    for (l4_uint64_t i = 0; i < qop->block_size; i++) {
        Queue_Obj* qo = &qob->start[i];
        qo->used = false;
        qo->block = qob;
        qo->data.q_ctx = (Q_CTX*)malloc(sizeof(Q_CTX));
    }

    add_block_to_pool(qop, qob);
}

inline void index_queue(Queue_Obj_Pool* qop, Queue_Obj* qo) {
    qop->data.index[qo->data.id] = qo;
}

inline Queue_Obj* get_queue(Queue_Obj_Pool* qop, l4_uint32_t id) {
    return qop->data.index[id];
}

void free_block(Queue_Obj_Pool* qop, Queue_Obj_Block* qob);

Queue_Obj* alloc_queue(Queue_Obj_Pool* qop, l4_size_t size);

void free_queue(Queue_Obj_Pool* qop, l4_uint64_t id);

inline void remove_block_from_pool(Queue_Obj_Pool* qop, Queue_Obj_Block* qob) {
    PA::remove_block_from_pool<QOP_PD, QOP_BD, QOP_ED>(qop, qob);
}

inline void add_block_to_pool(Queue_Obj_Pool* qop, Queue_Obj_Block* qob) {
    PA::add_block_to_pool<QOP_PD, QOP_BD, QOP_ED>(qop, qob);
}

inline Queue_Obj_Block* create_block(Queue_Obj_Pool* qop) {
    return PA::create_block<QOP_PD, QOP_BD, QOP_ED>(qop);
}

inline void destroy_block(Queue_Obj_Block* qob) {
    PA::destroy_block<QOP_BD, QOP_ED>(qob);
}

}