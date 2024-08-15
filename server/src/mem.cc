#include <cstdio>
#include <l4/re/mem_alloc>
#include <l4/re/util/cap_alloc>
#include <l4/re/error_helper>
#include <l4/vbus/vbus_pci>
#include <l4/re/cap_alloc>
#include "mem.h"
#include "l4/re/dataspace"
#include "l4/re/util/shared_cap"
#include "l4/sys/cxx/capability.h"

using namespace MEM;

void MEM::alloc_dma_mem(L4Re::Util::Shared_cap<L4Re::Dma_space>& dma_cap, l4_size_t size, DMA_MEM* dma_mem) {
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
}

void MEM::free_dma_mem(L4Re::Util::Shared_cap<L4Re::Dma_space>& dma_cap, l4_size_t size, DMA_MEM* dma_mem) {


    L4Re::chksys(dma_cap->unmap(dma_mem->phys, size, L4Re::Dma_space::Attributes::None, L4Re::Dma_space::Direction::Bidirectional));

    //TODO free DMA memory
    /*L4::Cap<L4Re::Dataspace> dc = L4::Ipc::make_cap_rw(dma_mem->cap.get()).cap();
    L4Re::chksys(L4Re::Env::env()->rm()->detach(dma_mem->virt, &dc));*/

    dma_mem->cap.get().invalidate();
    dma_mem->cap.release();
}

void MEM::alloc_block(MEM_Page_Pool* mpp) {
    MEM_Page_Block* mpb = create_block(mpp);

    alloc_dma_mem(*mpp->data.dma_cap, HCA_PAGE_SIZE * mpp->block_size, &mpb->data.dma_mem);
    MEM_Page* mp;
    for (l4_uint64_t i = 0; i < mpp->block_size; i++) {
        mp = &mpb->start[i];
        mp->used = false;
        mp->block = mpb;
        mp->data.virt = (l4_uint8_t*)mpb->data.dma_mem.virt + (i * HCA_PAGE_SIZE);
        mp->data.phys = mpb->data.dma_mem.phys + (i * HCA_PAGE_SIZE);
    }

    add_block_to_pool(mpp, mpb);
}

void MEM::free_block(MEM_Page_Pool* mpp, MEM_Page_Block* mpb) {
    remove_block_from_pool(mpp, mpb);

    free_dma_mem(*mpp->data.dma_cap, HCA_PAGE_SIZE * mpp->block_size, &mpb->data.dma_mem);

    destroy_block(mpb);
}
