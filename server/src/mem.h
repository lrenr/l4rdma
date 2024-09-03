#pragma once

#include <unordered_map>
#include <l4/re/env>
#include <l4/re/dma_space>
#include <l4/re/dataspace>
#include <l4/re/util/shared_cap>
#include "pool.h"

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

struct MEM_PD;

struct MEM_BD {
    DMA_MEM dma_mem;
};

struct MEM_ED {
    void* virt;
    L4Re::Dma_space::Dma_addr phys;
};

typedef PA::Pool<MEM_PD, MEM_BD, MEM_ED> MEM_Page_Pool;
typedef PA::Block<MEM_BD, MEM_ED> MEM_Page_Block;
typedef PA::Element<MEM_BD, MEM_ED> MEM_Page;

struct MEM_PD {
    dma* dma_cap;
    std::unordered_map<l4_uint64_t, MEM_Page*> index;
};

void alloc_block(MEM_Page_Pool* mpp);

void free_block(MEM_Page_Pool* mpp, MEM_Page_Block* mpb);

inline MEM_Page* alloc_page(MEM_Page_Pool* mpp) {
    MEM_Page* mp = PA::alloc_page<MEM_PD, MEM_BD, MEM_ED>(mpp);
    mpp->data.index[mp->data.phys] = mp;
    return mp;
}

inline void free_page(MEM_Page_Pool* mpp, l4_uint64_t phys) {
    auto n = mpp->data.index.extract(phys);
    PA::free_page<MEM_PD, MEM_BD, MEM_ED>(mpp, n.mapped());
}

inline void remove_block_from_pool(MEM_Page_Pool* mpp, MEM_Page_Block* mpb) {
    PA::remove_block_from_pool<MEM_PD, MEM_BD, MEM_ED>(mpp, mpb);
}

inline void add_block_to_pool(MEM_Page_Pool* mpp, MEM_Page_Block* mpb) {
    PA::add_block_to_pool<MEM_PD, MEM_BD, MEM_ED>(mpp, mpb);
}

inline MEM_Page_Block* create_block(MEM_Page_Pool* mpp) {
    return PA::create_block<MEM_PD, MEM_BD, MEM_ED>(mpp);
}

inline void destroy_block(MEM_Page_Block* mpb) {
    PA::destroy_block<MEM_BD, MEM_ED>(mpb);
}

void alloc_dma_mem(L4Re::Util::Shared_cap<L4Re::Dma_space>& dma_cap, l4_size_t size, DMA_MEM* dma_mem);

void free_dma_mem(L4Re::Util::Shared_cap<L4Re::Dma_space>& dma_cap, l4_size_t size, DMA_MEM* dma_mem);

}
