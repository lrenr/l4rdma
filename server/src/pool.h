#pragma once

#include <cstdio>
#include <cstdlib>
#include <l4/re/env>
#include <mutex>

/* Pool Allocator */
namespace PA {

/* Element that holds data */
template<typename BD, typename ED>
struct Element;

/* Block with Pool.block_size number of Elements */
template<typename BD, typename ED>
struct Block;

/* Pool of dynamically allocated number of Blocks */
template<typename PD, typename BD, typename ED>
struct Pool;

template<typename BD, typename ED>
struct Element {
    bool used;
    ED data;
    Block<BD, ED>* block;
};

template<typename BD, typename ED>
struct Block {
    l4_uint64_t page_count;
    Block* next;
    Block* prev;
    BD data;
    Element<BD, ED>* start;
};

template<typename PD, typename BD, typename ED>
struct Pool {
    std::mutex lock;
    l4_uint64_t max;
    l4_uint64_t size;
    l4_uint64_t block_size;
    l4_uint64_t block_count;
    void (*alloc_block)(Pool<PD, BD, ED>*);
    void (*free_block)(Pool<PD, BD, ED>*, Block<BD, ED>*);
    PD data;
    Block<BD, ED>* start;
    Block<BD, ED>* end;
};

/* gets an available Element from the Pool */
template<typename PD, typename BD, typename ED>
Element<BD, ED>* alloc_page(Pool<PD, BD, ED>* pp) {
    std::lock_guard<std::mutex> lock(pp->lock);

    if (pp->size == pp->max) return nullptr;
    if (pp->block_count == 0) pp->alloc_block(pp);
    Block<BD, ED>* pb = pp->start;
    while (true) {
        if (pb->page_count < pp->block_size) {
            for (l4_uint64_t i = 0; i < pp->block_size; i++) {
                if (pb->start[i].used) continue;
                pp->size++;
                pb->page_count++;
                pb->start[i].used = true;

                return &pb->start[i];
            }
        }
        if (pb->next == nullptr) pp->alloc_block(pp);
        pb = pb->next;
    }
}

/* returns an unused Element to the Pool */
template<typename PD, typename BD, typename ED>
void free_page(Pool<PD, BD, ED>* pp, Element<BD, ED>* p) {
    std::lock_guard<std::mutex> lock(pp->lock);

    Block<BD, ED>* pb = p->block;
    pp->size--;
    pb->page_count--;
    p->used = false;
    if (pb->page_count == 0) pp->free_block(pp, pb);
}

/* removes Block from Pool Block List */
template<typename PD, typename BD, typename ED>
void remove_block_from_pool(Pool<PD, BD, ED>* pp, Block<BD, ED>* pb) {
    if (pb->prev == nullptr) {
        if(pb->next == nullptr) pp->start = nullptr;
        else pp->start = pb->next;
    }
    else if (pb->next == nullptr) pb->prev->next = nullptr;
    else {
        pb->prev->next = pb->next;
        pb->next->prev = pb->prev;
    }
    pp->block_count--;
}

/* adds Block to Pool Block List */
template<typename PD, typename BD, typename ED>
void add_block_to_pool(Pool<PD, BD, ED>* pp, Block<BD, ED>* pb) {
    if (pp->start == nullptr) {
        pp->start = pb;
        pp->end = pb;
    }
    else {
        pp->end->next = pb;
        pb->prev = pp->end;
        pp->end = pb;
    }
    pp->block_count++;
}

/* allocates and initializes new Block */
template<typename PD, typename BD, typename ED>
Block<BD, ED>* create_block(Pool<PD, BD, ED>* pp) {
    Block<BD, ED>* pb = (Block<BD, ED>*)malloc(sizeof(Block<BD, ED>));
    pb->page_count = 0;
    pb->next = nullptr;
    pb->prev = nullptr;
    pb->start = (Element<BD, ED>*)malloc(sizeof(Element<BD, ED>) * pp->block_size);
    return pb;
}

/* frees unused Block */
template<typename BD, typename ED>
void destroy_block(Block<BD, ED>* pb) {
    free(pb->start);
    free(pb);
}

}