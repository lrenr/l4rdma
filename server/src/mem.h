#pragma once

#include <unordered_map>
#include <l4/re/env>
#include <l4/re/dma_space>
#include <l4/re/dataspace>
#include <l4/re/util/shared_cap>
#include "page.h"

typedef const l4_uint32_t cu32;
typedef volatile l4_uint32_t reg32;
typedef L4Re::Util::Shared_cap<L4Re::Dma_space> dma;

cu32 HCA_PAGE_SIZE = 4096;

namespace MEM {

struct DMA_MEM {
    L4Re::Util::Shared_cap<L4Re::Dataspace> cap;
    void* virt;
    L4Re::Dma_space::Dma_addr phys;
};

struct MEM_PPD;

struct MEM_BD {
    DMA_MEM dma_mem;
};

struct MEM_PD {
    void* virt;
    L4Re::Dma_space::Dma_addr phys;
};

typedef PA::Page_Pool<MEM_PPD, MEM_BD, MEM_PD> MEM_Page_Pool;
typedef PA::Page_Block<MEM_BD, MEM_PD> MEM_Page_Block;
typedef PA::Page<MEM_BD, MEM_PD> MEM_Page;

struct MEM_PPD {
    dma* dma_cap;
    std::unordered_map<l4_uint64_t, MEM_Page*> index;
};

void alloc_block(MEM_Page_Pool* mpp);

void free_block(MEM_Page_Pool* mpp, MEM_Page_Block* mpb);

inline MEM_Page* alloc_page(MEM_Page_Pool* mpp) {
    MEM_Page* mp = PA::alloc_page<MEM_PPD, MEM_BD, MEM_PD>(mpp);
    mpp->data.index[mp->data.phys] = mp;
    return mp;
}

inline void free_page(MEM_Page_Pool* mpp, l4_uint64_t phys) {
    auto n = mpp->data.index.extract(phys);
    PA::free_page<MEM_PPD, MEM_BD, MEM_PD>(mpp, n.mapped());
}

inline void remove_block_from_pool(MEM_Page_Pool* mpp, MEM_Page_Block* mpb) {
    PA::remove_block_from_pool<MEM_PPD, MEM_BD, MEM_PD>(mpp, mpb);
}

inline void add_block_to_pool(MEM_Page_Pool* mpp, MEM_Page_Block* mpb) {
    PA::add_block_to_pool<MEM_PPD, MEM_BD, MEM_PD>(mpp, mpb);
}

inline MEM_Page_Block* create_block(MEM_Page_Pool* mpp) {
    return PA::create_block<MEM_PPD, MEM_BD, MEM_PD>(mpp);
}

inline void destroy_block(MEM_Page_Block* mpb) {
    PA::destroy_block<MEM_BD, MEM_PD>(mpb);
}

template<typename QE>
struct Queue {
    QE* start;
    l4_size_t size;
    l4_uint32_t head = 0;
    l4_uint32_t id = 0;
    DMA_MEM dma_mem;
};

template<typename QE>
l4_uint32_t enqueue(Queue<QE>& q) {
    l4_uint32_t slot = q.head;
    if (++q.head >= q.size)
        q.head = 0;
    return slot;
}

void alloc_dma_mem(L4Re::Util::Shared_cap<L4Re::Dma_space>& dma_cap, l4_size_t size, DMA_MEM* dma_mem);

void free_dma_mem(L4Re::Util::Shared_cap<L4Re::Dma_space>& dma_cap, l4_size_t size, DMA_MEM* dma_mem);

}
