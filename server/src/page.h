#pragma once

#include <cstdio>
#include <cstdlib>
#include <l4/re/env>
#include <mutex>

/* Page Allocator */
namespace PA {

/* Page that holds pointer to some memory */
template<typename BD, typename PD>
struct Page;

/* Block of Page_Pool.block_size number of Pages */
template<typename BD, typename PD>
struct Page_Block;

/* Pool of dynamically allocated number of Page Blocks */
template<typename PPD, typename BD, typename PD>
struct Page_Pool;

template<typename BD, typename PD>
struct Page {
    bool used;
    PD data;
    Page_Block<BD, PD>* block;
};

template<typename BD, typename PD>
struct Page_Block {
    l4_uint64_t page_count;
    Page_Block* next;
    Page_Block* prev;
    BD data;
    Page<BD, PD> *start;
};

template<typename PPD, typename BD, typename PD>
struct Page_Pool {
    std::mutex lock;
    l4_uint64_t max;
    l4_uint64_t size;
    l4_uint64_t block_size;
    l4_uint64_t block_count;
    void (*alloc_block)(Page_Pool<PPD, BD, PD>*);
    void (*free_block)(Page_Pool<PPD, BD, PD>*, Page_Block<BD, PD>*);
    PPD data;
    Page_Block<BD, PD>* start;
};

/* gets an available Page from the Pool */
template<typename PPD, typename BD, typename PD>
Page<BD, PD>* alloc_page(Page_Pool<PPD, BD, PD>* pp) {
    std::lock_guard<std::mutex> lock(pp->lock);

    if (pp->size == pp->max) return nullptr;
    if (pp->block_count == 0) pp->alloc_block(pp);
    Page_Block<BD, PD>* pb = pp->start;
    while (true) {
        if (pb->page_count < pp->block_size) {
            Page<BD, PD>* p = pb->start;
            for (l4_uint64_t i = 0; i < pp->block_size; i++) {
                if (p[i].used) continue;
                pp->size++;
                pb->page_count++;
                p[i].used = true;

                return &p[i];
            }
        }
        if (pb->next == nullptr) pp->alloc_block(pp);
        pb = pb->next;
    }
}

/* returns an unused Page to the Pool */
template<typename PPD, typename BD, typename PD>
void free_page(Page_Pool<PPD, BD, PD>* pp, Page<BD, PD>* p) {
    std::lock_guard<std::mutex> lock(pp->lock);

    Page_Block<BD, PD>* pb = p->block;
    pp->size--;
    pb->page_count--;
    p->used = false;
    if (pb->page_count == 0) pp->free_block(pp, pb);
}

/* removes Page Block from Pool Block List */
template<typename PPD, typename BD, typename PD>
void remove_block_from_pool(Page_Pool<PPD, BD, PD>* pp, Page_Block<BD, PD>* pb) {
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

/* adds Page Block to Pool Block List */
template<typename PPD, typename BD, typename PD>
void add_block_to_pool(Page_Pool<PPD, BD, PD>* pp, Page_Block<BD, PD>* pb) {
    if (pp->start == nullptr) pp->start = pb;
    else {
        Page_Block<BD, PD>* current = pp->start;
        while (true) {
            if (current->next == nullptr) {
                current->next = pb;
                pb->prev = current;
                break;
            }
            current = current->next;
        }
    }
    pp->block_count++;
}

/* allocates and initializes new Page Block */
template<typename PPD, typename BD, typename PD>
Page_Block<BD, PD>* create_block(Page_Pool<PPD, BD, PD>* pp) {
    Page_Block<BD, PD>* pb = (Page_Block<BD, PD>*)malloc(sizeof(Page_Block<BD, PD>));
    pb->page_count = 0;
    pb->next = nullptr;
    pb->prev = nullptr;
    pb->start = (Page<BD, PD>*)malloc(sizeof(Page<BD, PD>) * pp->block_size);
    return pb;
}

/* frees unused Page Block */
template<typename BD, typename PD>
void destroy_block(Page_Block<BD, PD>* pb) {
    free(pb->start);
    free(pb);
}

}