#include <stdio.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include "interface.h"
#include "device.h"
#include "driver.h"
#include "cmd.h"
#include "mem.h"

Registry main_srv;
L4RDMA_Server l4rdma_server;
 
int main(int argc, char **argv) {
	
	if (!main_srv.registry()->register_obj(&l4rdma_server, "l4rdma_server").is_valid()) {
		printf("Could not register service\n");
		return 1;
	}

	printf("Welcome to the l4rdma server\n");

	L4::Cap<L4vbus::Vbus> vbus = L4Re::Env::env()->get_cap<L4vbus::Vbus>("vbus_mlx");
	if (!vbus.is_valid()) throw;
	L4vbus::Pci_dev dev = Device::pci_get_dev(vbus);
	l4_uint8_t* bar0 = Device::map_pci_bar(dev, 0);
	printf("bar0:%p\n", bar0);

	/* MSI-X Setup */
	l4_uint32_t table_offset, bir, table_size;
	if (Device::setup_msix(dev, table_offset, bir, table_size)) throw;
	if (table_size + 1 < 10) throw;
	/* map BAR that contains the MSI-X table */
    l4_uint8_t* table_bar;
	if(bir == 0) table_bar = bar0;
	else table_bar = Device::map_pci_bar(dev, bir);
	reg32* msix_table = (reg32*)(table_bar + table_offset);
	L4::Cap<L4::Icu> icu;
	Device::create_icu_cap(dev, icu);
	l4_uint64_t icu_src = dev.dev_handle() | L4vbus::Icu::Src_dev_handle;

	CMD::CMD_Args cmd_args;
	dma dma_cap;
	MEM::HCA_DMA_MEM hca_dma_mem;

	dma_cap = Device::bind_dma_space_to_device(dev);
	MEM::DMA_MEM* cq_mem = MEM::alloc_dma_mem(dma_cap, HCA_PAGE_SIZE, &hca_dma_mem.dma_mem[0]);
	cmd_args.imb_mem = MEM::alloc_dma_mem(dma_cap, HCA_PAGE_SIZE * CMD::MBB_MAX_COUNT, &hca_dma_mem.dma_mem[1]);
	cmd_args.omb_mem = MEM::alloc_dma_mem(dma_cap, HCA_PAGE_SIZE * CMD::MBB_MAX_COUNT, &hca_dma_mem.dma_mem[2]);
	hca_dma_mem.dma_mem_count += 3;

	MEM::MEM_Page_Pool mem_page_pool;
    mem_page_pool.max = 102400;
    mem_page_pool.size = 0;
    mem_page_pool.block_size = 64;
    mem_page_pool.block_count = 0;
    mem_page_pool.alloc_block = MEM::alloc_block;
    mem_page_pool.free_block = MEM::free_block;
    mem_page_pool.meta.dma_cap = &dma_cap;
    mem_page_pool.start = nullptr;

	MEM::MEM_Page* mp = MEM::alloc_page(&mem_page_pool);
	MEM::free_page(&mem_page_pool, mp->meta.phys);

	UAR::UAR_Page_Pool uar_page_pool;
	uar_page_pool.max = 1024;
    uar_page_pool.size = 0;
    uar_page_pool.block_size = 64;
    uar_page_pool.block_count = 0;
    uar_page_pool.alloc_block = UAR::alloc_block;
    uar_page_pool.free_block = UAR::free_block;
    uar_page_pool.meta.base.index = 4096;
	uar_page_pool.meta.base.addr = (UAR::Page*)(bar0 + (HCA_PAGE_SIZE * 4096));
    uar_page_pool.start = nullptr;

	UAR::UAR_Page* up = UAR::alloc_page(&uar_page_pool);
	UAR::free_page(&uar_page_pool, up);

	using namespace Driver;
	Init_Seg* init_seg = (Driver::Init_Seg*)bar0;
	cmd_args.dbv = &init_seg->dbv;
	cmd_args.cq.size = 32;
	cmd_args.cq.start = (CMD::CQE*)cq_mem->virt;
	
	printf("------------\n\n");
	init_hca(cmd_args, dma_cap, init_seg, cq_mem, hca_dma_mem);
	
	printf("------------\n\n");
	UAR::UAR uar = alloc_uar(cmd_args, bar0);
	setup_event_queue(cmd_args, icu_src, msix_table, icu, hca_dma_mem, dma_cap, uar);

	printf("------------\n\n");
	teardown_hca(cmd_args);

	main_srv.loop();

	return 0;
}
