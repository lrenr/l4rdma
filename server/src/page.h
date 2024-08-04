#pragma once

#include <cstdlib>
#include <l4/re/env>
#include <mutex>

/* Page Allocator */
namespace PA {

/* Page that holds pointer to some memory */
template<typename BM, typename PM>
struct Page;

/* Block of Page_Pool.block_size number of Pages */
template<typename BM, typename PM>
struct Page_Block;

/* Pool of dynamically allocated number of Page Blocks */
template<typename PPM, typename BM, typename PM>
struct Page_Pool;

template<typename BM, typename PM>
struct Page {
    bool used;
    PM meta;
    Page_Block<BM, PM>* block;
};

template<typename BM, typename PM>
struct Page_Block {
    l4_uint64_t page_count;
    Page_Block* next;
    Page_Block* prev;
    BM meta;
    Page<BM, PM> *start;
};

template<typename PPM, typename BM, typename PM>
struct Page_Pool {
    std::mutex lock;
    l4_uint64_t max;
    l4_uint64_t size;
    l4_uint64_t block_size;
    l4_uint64_t block_count;
    void (*alloc_block)(Page_Pool<PPM, BM, PM>*);
    void (*free_block)(Page_Pool<PPM, BM, PM>*, Page_Block<BM, PM>*);
    PPM meta;
    Page_Block<BM, PM>* start;
};

/* gets an available Page from the Pool */
template<typename PPM, typename BM, typename PM>
Page<BM, PM>* alloc_page(Page_Pool<PPM, BM, PM>* pp) {
    std::lock_guard<std::mutex> lock(pp->lock);

    if (pp->size == pp->max) return nullptr;
    if (pp->block_count == 0) pp->alloc_block(pp);
    Page_Block<BM, PM>* pb = pp->start;
    while (true) {
        if (pb->page_count < pp->block_size) {
            Page<BM, PM>* p = pb->start;
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
template<typename PPM, typename BM, typename PM>
void free_page(Page_Pool<PPM, BM, PM>* pp, Page<BM, PM>* p) {
    std::lock_guard<std::mutex> lock(pp->lock);

    Page_Block<BM, PM>* pb = p->block;
    pp->size--;
    pb->page_count--;
    p->used = false;
    if (pb->page_count == 0) pp->free_block(pp, pb);
}

/* removes Page Block from Pool Block List */
template<typename PPM, typename BM, typename PM>
void remove_block_from_pool(Page_Pool<PPM, BM, PM>* pp, Page_Block<BM, PM>* pb) {
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
template<typename PPM, typename BM, typename PM>
void add_block_to_pool(Page_Pool<PPM, BM, PM>* pp, Page_Block<BM, PM>* pb) {
    if (pp->start == nullptr) pp->start = pb;
    else {
        Page_Block<BM, PM>* current = pp->start;
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
template<typename PPM, typename BM, typename PM>
Page_Block<BM, PM>* create_block(Page_Pool<PPM, BM, PM>* pp) {
    Page_Block<BM, PM>* pb = (Page_Block<BM, PM>*)malloc(sizeof(Page_Block<BM, PM>));
    pb->page_count = 0;
    pb->next = nullptr;
    pb->prev = nullptr;
    pb->start = (Page<BM, PM>*)malloc(sizeof(Page<BM, PM>) * pp->block_size);
    return pb;
}

/* frees unused Page Block */
template<typename BM, typename PM>
void destroy_block(Page_Block<BM, PM>* pb) {
    free(pb->start);
    free(pb);
}

}