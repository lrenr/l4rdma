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
	l4_uint8_t *bar0 = Device::map_pci_bar(dev, 0);
	printf("bar0:%p\n", bar0);

	CMD::CQ cq;
	dma dma_cap;
	MEM::HCA_DMA_MEM hca_dma_mem;

	dma_cap = Device::bind_dma_space_to_device(dev);
	MEM::DMA_MEM* cq_mem = MEM::alloc_dma_mem(dma_cap, HCA_PAGE_SIZE, &hca_dma_mem.dma_mem[0]);
	MEM::DMA_MEM* imb_mem = MEM::alloc_dma_mem(dma_cap, HCA_PAGE_SIZE * CMD::MBB_MAX_COUNT, &hca_dma_mem.dma_mem[1]);
	MEM::DMA_MEM* omb_mem = MEM::alloc_dma_mem(dma_cap, HCA_PAGE_SIZE * CMD::MBB_MAX_COUNT, &hca_dma_mem.dma_mem[2]);
	hca_dma_mem.dma_mem_count += 3;

	using namespace Driver;
	Init_Seg* init_seg = (Driver::Init_Seg*)bar0;
	cq.size = 32;
	cq.start = (CMD::CQE*)cq_mem->virt;
	init_hca(cq, dma_cap, init_seg, cq_mem, imb_mem, omb_mem, hca_dma_mem);
	teardown_hca(cq, init_seg, omb_mem);

	main_srv.loop();

	return 0;
}
