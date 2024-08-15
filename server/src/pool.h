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
    Block<BD, ED>* end; // when start == nullptr undefined
};

/* gets an available Element from the Pool */
template<typename PD, typename BD, typename ED>
Element<BD, ED>* alloc_page(Pool<PD, BD, ED>* p) {
    std::lock_guard<std::mutex> lock(p->lock);

    if (p->size == p->max) return nullptr;
    if (p->block_count == 0) p->alloc_block(p);
    Block<BD, ED>* b = p->start;
    while (true) {
        if (b->page_count < p->block_size) {
            for (l4_uint64_t i = 0; i < p->block_size; i++) {
                if (b->start[i].used) continue;
                p->size++;
                b->page_count++;
                b->start[i].used = true;

                return &b->start[i];
            }
        }
        if (b->next == nullptr) p->alloc_block(p);
        b = b->next;
    }
}

/* returns an unused Element to the Pool */
template<typename PD, typename BD, typename ED>
void free_page(Pool<PD, BD, ED>* p, Element<BD, ED>* e) {
    std::lock_guard<std::mutex> lock(p->lock);

    Block<BD, ED>* b = e->block;
    p->size--;
    b->page_count--;
    e->used = false;
    if (b->page_count == 0) p->free_block(p, b);
}

/* removes Block from Pool Block List */
template<typename PD, typename BD, typename ED>
void remove_block_from_pool(Pool<PD, BD, ED>* p, Block<BD, ED>* b) {
    if (b->prev == nullptr) {
        if(b->next == nullptr) p->start = nullptr;
        else p->start = b->next;
    }
    else if (b->next == nullptr) {
        b->prev->next = nullptr;
        p->end = b->prev;
    }
    else {
        b->prev->next = b->next;
        b->next->prev = b->prev;
    }
    p->block_count--;
}

/* adds Block to Pool Block List */
template<typename PD, typename BD, typename ED>
void add_block_to_pool(Pool<PD, BD, ED>* p, Block<BD, ED>* b) {
    if (p->start == nullptr) {
        p->start = b;
        p->end = b;
    }
    else {
        p->end->next = b;
        b->prev = p->end;
        p->end = b;
    }
    p->block_count++;
}

/* allocates and initializes new Block */
template<typename PD, typename BD, typename ED>
Block<BD, ED>* create_block(Pool<PD, BD, ED>* p) {
    Block<BD, ED>* b = (Block<BD, ED>*)malloc(sizeof(Block<BD, ED>));
    b->page_count = 0;
    b->next = nullptr;
    b->prev = nullptr;
    b->start = (Element<BD, ED>*)malloc(sizeof(Element<BD, ED>) * p->block_size);
    return b;
}

/* frees unused Block */
template<typename BD, typename ED>
void destroy_block(Block<BD, ED>* b) {
    free(b->start);
    free(b);
}

}