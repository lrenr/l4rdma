#include <stdio.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include "interface.h"
#include "device.h"
#include "hca.h"
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
	MEM::HCA_PAGE_MEM hca_page_mem;

	dma_cap = Device::bind_dma_space_to_device(dev);
	MEM::DMA_MEM cq_mem = MEM::alloc_dma_mem(dma_cap, HCA_PAGE_SIZE);
	MEM::DMA_MEM imb_mem = MEM::alloc_dma_mem(dma_cap, HCA_PAGE_SIZE * CMD::MBB_MAX_COUNT);
	MEM::DMA_MEM omb_mem = MEM::alloc_dma_mem(dma_cap, HCA_PAGE_SIZE * CMD::MBB_MAX_COUNT);

	using namespace HCA;
	Init_Seg* init_seg = (HCA::Init_Seg*)bar0;
	init_hca(cq, dma_cap, hca_page_mem, init_seg, cq_mem, imb_mem, omb_mem);
	//teardown_hca(init_seg);

	main_srv.loop();

	return 0;
}
