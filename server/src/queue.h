#pragma once

#include <l4/re/env>
#include <l4/re/dma_space>
#include <l4/re/dataspace>
#include <l4/re/util/shared_cap>
#include "mem.h"
#include "uar.h"

namespace Q {

template<typename QE>
struct Queue {
    QE* start;
    l4_size_t size;
    l4_uint32_t head = 0;
    MEM::DMA_MEM dma_mem;
};

template<typename QE>
l4_uint32_t enqueue(Queue<QE>& q) {
    l4_uint32_t slot = q.head;
    if (++q.head >= q.size)
        q.head = 0;
    return slot;
}

struct QOP_PD;

struct QOP_BD {};

struct QOP_ED : Q::Queue<void> {
    l4_uint32_t id;
    UAR::UAR_Page* uarp;
};

typedef PA::Pool<QOP_PD, QOP_BD, QOP_ED> Queue_Obj_Pool;
typedef PA::Block<QOP_BD, QOP_ED> Queue_Obj_Block;
typedef PA::Element<QOP_BD, QOP_ED> Queue_Obj;

struct QOP_PD {
    std::unordered_map<l4_uint64_t, Queue_Obj*> index;
};

void alloc_block(Queue_Obj_Pool* qop);

void free_block(Queue_Obj_Pool* qop, Queue_Obj_Block* qob);

inline Queue_Obj* alloc_queue(Queue_Obj_Pool* qop) {
    Queue_Obj* qo = PA::alloc_page<QOP_PD, QOP_BD, QOP_ED>(qop);
    qop->data.index[qo->data.id] = qo;
    return qo;
}

inline void free_queue(Queue_Obj_Pool* qop, l4_uint64_t id) {
    auto n = qop->data.index.extract(id);
    PA::free_page<QOP_PD, QOP_BD, QOP_ED>(qop, n.mapped());
}

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