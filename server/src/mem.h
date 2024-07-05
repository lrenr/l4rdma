#pragma once

#include <l4/re/env>
#include <l4/re/dma_space>
#include <l4/re/dataspace>
#include <l4/re/util/shared_cap>

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

struct HCA_DMA_MEM {
    DMA_MEM dma_mem[64];
    l4_uint32_t dma_mem_count = 0;
};

DMA_MEM* alloc_dma_mem(L4Re::Util::Shared_cap<L4Re::Dma_space> &dma_cap, l4_size_t size, DMA_MEM* dma_mem);

}
