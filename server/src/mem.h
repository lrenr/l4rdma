#pragma once

#include <l4/re/env>
#include <l4/re/dma_space>
#include <l4/re/dataspace>
#include <l4/re/util/shared_cap>

typedef l4_uint32_t reg32;
typedef l4_uint64_t reg64;

namespace MEM {

struct DMA_MEM {
    L4Re::Util::Shared_cap<L4Re::Dataspace> cap;
    void* virt;
    L4Re::Dma_space::Dma_addr phys;
};

DMA_MEM alloc_dma_mem(L4Re::Util::Shared_cap<L4Re::Dma_space>& dma_cap, l4_size_t size);

}