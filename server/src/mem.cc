#include <l4/re/mem_alloc>
#include <l4/re/util/cap_alloc>
#include <l4/re/error_helper>
#include <l4/vbus/vbus_pci>
#include "mem.h"
#include "l4/sys/consts.h"

using namespace MEM;

DMA_MEM MEM::alloc_dma_mem(L4Re::Util::Shared_cap<L4Re::Dma_space>& dma_cap, l4_size_t size) {
    DMA_MEM dma_mem;
    dma_mem.cap = L4Re::chkcap(L4Re::Util::make_shared_cap<L4Re::Dataspace>(),
        "Failed to allocate DMA Dataspace");

    if (size % L4_SUPERPAGESIZE) {
        size = ((size >> L4_SUPERPAGESHIFT) + 1) << L4_SUPERPAGESHIFT;
    }

    L4Re::chksys(L4Re::Env::env()->mem_alloc()->alloc(size, dma_mem.cap.get(),
        L4Re::Mem_alloc::Continuous | L4Re::Mem_alloc::Pinned),
        "Failed to allocate DMA memory.");

    L4Re::chksys(L4Re::Env::env()->rm()->attach(&dma_mem.virt, size,
        L4Re::Rm::F::Search_addr | L4Re::Rm::F::RW,
        L4::Ipc::make_cap_rw(dma_mem.cap.get()), 0, L4_SUPERPAGESHIFT),
        "Failed to attach DMA memory");

    L4Re::chksys(dma_cap->map(L4::Ipc::make_cap_rw(dma_mem.cap.get()), 0, &size,
    L4Re::Dma_space::Attributes::None, L4Re::Dma_space::Direction::Bidirectional, &dma_mem.phys),
    "Failed to setup memory region for DMA.");

    return dma_mem;
}