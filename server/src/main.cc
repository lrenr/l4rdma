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

	Driver::MLX5_Context ctx;
	dma dma_cap;

	dma_cap = Device::bind_dma_space_to_device(dev);
	MEM::alloc_dma_mem(dma_cap, HCA_PAGE_SIZE, &ctx.cmd_args.cq.dma_mem);
	MEM::alloc_dma_mem(dma_cap, HCA_PAGE_SIZE * CMD::MBB_MAX_COUNT, &ctx.cmd_args.imb_mem);
	MEM::alloc_dma_mem(dma_cap, HCA_PAGE_SIZE * CMD::MBB_MAX_COUNT, &ctx.cmd_args.omb_mem);

	ctx.mem_page_pool.max = 65536;
	ctx.mem_page_pool.size = 0;
	ctx.mem_page_pool.block_size = 1024;
	ctx.mem_page_pool.block_count = 0;
	ctx.mem_page_pool.alloc_block = MEM::alloc_block;
	ctx.mem_page_pool.free_block = MEM::free_block;
	ctx.mem_page_pool.data.dma_cap = &dma_cap;
	ctx.mem_page_pool.start = nullptr;

	MEM::MEM_Page* mp = MEM::alloc_page(&ctx.mem_page_pool);
	MEM::free_page(&ctx.mem_page_pool, mp->data.phys);

	ctx.uar_page_pool.max = 1024;
	ctx.uar_page_pool.size = 0;
	ctx.uar_page_pool.block_size = 64;
	ctx.uar_page_pool.block_count = 0;
	ctx.uar_page_pool.alloc_block = UAR::alloc_block;
	ctx.uar_page_pool.free_block = UAR::free_block;
	ctx.uar_page_pool.data.base.index = 4096;
	ctx.uar_page_pool.data.base.addr = (UAR::Page*)(bar0 + (HCA_PAGE_SIZE * 4096));
	ctx.uar_page_pool.start = nullptr;

	UAR::UAR_Page* up = UAR::alloc_page(&ctx.uar_page_pool);
	UAR::free_page(&ctx.uar_page_pool, up);
	up = UAR::alloc_page(&ctx.uar_page_pool);
	UAR::free_page(&ctx.uar_page_pool, up);

	using namespace Driver;
	Init_Seg* init_seg = (Driver::Init_Seg*)bar0;
	ctx.cmd_args.dbv = &init_seg->dbv;
	ctx.cmd_args.cq.size = 32;
	ctx.cmd_args.cq.start = (CMD::CQE*)ctx.cmd_args.cq.dma_mem.virt;

	printf("------------\n\n");
	init_hca(ctx, init_seg);
	printf("pool_size: %llu | pool_block_count: %llu | hash_map_size: %lu\n", ctx.mem_page_pool.size, ctx.mem_page_pool.block_count, ctx.mem_page_pool.data.index.size());

	printf("------------\n\n");
	//UAR::UAR uar = alloc_uar(ctx, bar0);
	//setup_event_queue(ctx, icu_src, msix_table, icu, dma_cap, uar);

	printf("------------\n\n");
	teardown_hca(ctx);

	main_srv.loop();

	return 0;
}
