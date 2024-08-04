#include <cstdio>
#include <l4/re/mem_alloc>
#include <l4/re/util/cap_alloc>
#include <l4/re/error_helper>
#include <l4/vbus/vbus_pci>
#include "mem.h"

using namespace MEM;

DMA_MEM* MEM::alloc_dma_mem(L4Re::Util::Shared_cap<L4Re::Dma_space>& dma_cap, l4_size_t size, DMA_MEM* dma_mem) {
    dma_mem->cap = L4Re::chkcap(L4Re::Util::make_shared_cap<L4Re::Dataspace>(),
        "Failed to allocate DMA Dataspace");

    if (size % L4_SUPERPAGESIZE) {
        size = ((size >> L4_SUPERPAGESHIFT) + 1) << L4_SUPERPAGESHIFT;
    }

    L4Re::chksys(L4Re::Env::env()->mem_alloc()->alloc(size, dma_mem->cap.get(),
        L4Re::Mem_alloc::Continuous | L4Re::Mem_alloc::Pinned),
        "Failed to allocate DMA memory.");
    
	L4Re::chksys(L4Re::Env::env()->rm()->attach(&dma_mem->virt, size,
        L4Re::Rm::F::Search_addr | L4Re::Rm::F::RW,
        L4::Ipc::make_cap_rw(dma_mem->cap.get()), 0, (unsigned char)L4_SUPERPAGESIZE),
        "Failed to attach DMA memory");

    L4Re::chksys(dma_cap->map(L4::Ipc::make_cap_rw(dma_mem->cap.get()), 0, &size,
    L4Re::Dma_space::Attributes::None, L4Re::Dma_space::Direction::Bidirectional, &dma_mem->phys),
    "Failed to setup memory region for DMA.");

    return dma_mem;
}

void MEM::alloc_block(MEM_Page_Pool* mpp) {
    MEM_Page_Block* mpb = create_block(mpp);

    alloc_dma_mem(*mpp->meta.dma_cap, HCA_PAGE_SIZE * mpp->block_size, &mpb->meta.dma_mem);
    for (l4_uint64_t i = 0; i < mpp->block_size; i++) {
        MEM_Page* mp = &mpb->start[i];
        mp->used = false;
        mp->block = mpb;
        mp->meta.virt = (l4_uint8_t*)mpb->meta.dma_mem.virt + (i * HCA_PAGE_SIZE);
        mp->meta.phys = mpb->meta.dma_mem.phys + (i * HCA_PAGE_SIZE);
    }

    add_block_to_pool(mpp, mpb);
}

void MEM::free_block(MEM_Page_Pool* mpp, MEM_Page_Block* mpb) {
    remove_block_from_pool(mpp, mpb);

    //TODO free DMA memory

    destroy_block(mpb);
}

MEM_Page* MEM::find_page(MEM_Page_Pool* mpp, l4_uint64_t phys) {
    MEM_Page_Block* mpb = mpp->start;
    MEM_Page* mp;
    while (true) {
        mp = mpb->start;
        for (l4_uint64_t i = 0; i < mpp->block_size; i++) {
            if (mp[i].meta.phys == phys) return &mp[i];
        }
        mpb = mpb->next;
    }
}
