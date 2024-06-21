#include <stdio.h>
#include <stdexcept>
#include <sys/ipc.h>
#include <sys/mman.h>
#include "interface.h"
#include "device.h"
#include "hca.h"
#include "cmd.h"
#include "mem.h"

CMD::CQ cq;
L4Re::Util::Shared_cap<L4Re::Dma_space> dma_cap;

void debug_cmd(volatile CMD::CQ &cq, l4_uint32_t slot) {
	reg32* entry = (reg32*)&cq.start[slot];
	for (int i = 0; i < 16; i++) printf("%.8x\n", Device::ioread32be(&entry[i]));
}

void ring_doorbell(reg32* dbv, l4_uint32_t* slots, int count) {
	l4_uint32_t dbr = 0;
	for (int i = 0; i < count; i++)
		dbr += (1 << slots[i]);
	printf("dbr: %.8x\n", dbr);
	Device::iowrite32be(dbv, dbr);
}

void init_wait(reg32* initializing) {
	l4_uint32_t init;
	while (true) {
		printf(".");
		init = Device::ioread32be(initializing);
		if (!(init >> 31)) break;
	}
	printf("\n");
}

void init_hca(HCA::Init_Seg* init_seg, MEM::DMA_MEM cq_mem) {
	using namespace Device;
	using namespace CMD;
	// create default Command Queue
	cq.size = 32;
	cq.start = (CQE*) cq_mem.virt;

	// read Firmware Version
	l4_uint32_t fw_rev = ioread32be(&init_seg->fw_rev);
	l4_uint32_t cmd_rev = ioread32be(&init_seg->cmdif_rev_fw_sub);
	l4_uint16_t fw_rev_major = fw_rev;
	l4_uint16_t fw_rev_minor = fw_rev >> 16;
	printf("Firmware Version: %.4x:%.4x %.4x:%.4x\n", fw_rev_major, fw_rev_minor,
		(l4_uint16_t)cmd_rev, (l4_uint16_t)(cmd_rev >> 16));

	init_wait(&init_seg->initializing);

	printf("\nInit_Seg\n");
	printf("%.8x\n", ioread32be(&init_seg->fw_rev));
	printf("%.8x\n", ioread32be(&init_seg->cmdif_rev_fw_sub));
	printf("%.8x\n", ioread32be(&init_seg->padding0[0]));
	printf("%.8x\n", ioread32be(&init_seg->padding0[1]));
	printf("%.8x\n", ioread32be(&init_seg->cmdq_addr_msb));
	printf("%.8x\n", ioread32be(&init_seg->cmdq_addr_lsb_info));
	printf("\n");

	l4_uint32_t info = ioread32be(&init_seg->cmdq_addr_lsb_info) & 0xff;
	l4_uint8_t log_size = info >> 4 &0xf;
	l4_uint8_t log_stride = info & 0xf;
	if (1 << log_size > 32) {
		printf("%d Commands in queue\n", 1 << log_size);
		throw;
	}
	if (log_size + log_stride > 12) {
		printf("cq size overflow\n");
		throw;
	}

	// write Command Queue Address to Device
	l4_uint64_t addr = cq_mem.phys;
	l4_uint32_t addr_lsb = addr & (~0x3ff);
	l4_uint32_t addr_msb = (l4_uint64_t)addr >> 32;
	addr_lsb = addr;
	addr_msb = (l4_uint64_t)addr >> 32;
	iowrite32be(&init_seg->cmdq_addr_msb, addr_msb);
	iowrite32be(&init_seg->cmdq_addr_lsb_info, addr_lsb);
	l4_uint32_t test1 = ioread32be(&init_seg->cmdq_addr_msb);
	l4_uint32_t test2 = ioread32be(&init_seg->cmdq_addr_lsb_info);
	printf("og: %.16llx\n", addr);
	printf("ref: %.8x:%.8x\n", addr_msb, addr_lsb);
	printf("addr_msb:addr_lsb: %.8x:%.8x\n", test1, test2);

	// wait for initialization
	init_wait(&init_seg->initializing);

	printf("\nInit_Seg\n");
	printf("%.8x\n", ioread32be(&init_seg->fw_rev));
	printf("%.8x\n", ioread32be(&init_seg->cmdif_rev_fw_sub));
	printf("%.8x\n", ioread32be(&init_seg->padding0[0]));
	printf("%.8x\n", ioread32be(&init_seg->padding0[1]));
	printf("%.8x\n", ioread32be(&init_seg->cmdq_addr_msb));
	printf("%.8x\n", ioread32be(&init_seg->cmdq_addr_lsb_info));
	printf("...\n");
	printf("%.8x\n", ioread32be(&init_seg->initializing));
	printf("\n");

	//TODO hardware health check

	// INIT SEQUENCE
	l4_uint32_t slot;
	//MEM::DMA_MEM imb_mem = MEM::alloc_dma_mem(dma_cap, 4096);
	MEM::DMA_MEM omb_mem = MEM::alloc_dma_mem(dma_cap, 4096);

	// ENABLE_HCA
	slot = create_cqe(cq, ENABLE_HCA, 0x00, nullptr, 0, nullptr, ENABLE_HCA_OUTPUT_LENGTH, nullptr);
	printf("slot: %d\n", slot);
	ring_doorbell(&init_seg->dbv, &slot, 1);
	validate_cqe(cq, slot);

	// QUERY_ISSI
	slot = create_cqe(cq, QUERY_ISSI, 0x00, nullptr, 0, nullptr, QUERY_ISSI_OUTPUT_LENGTH, &omb_mem);
	ring_doorbell(&init_seg->dbv, &slot, 1);
	validate_cqe(cq, slot);

	// Setting ISSI to Version 1 if supported or else 0
	l4_uint32_t issi = get_issi_support(omb_mem);
	if (issi) issi = 1;
	else throw std::runtime_error("ISSI version 1 not supported by Hardware");

	// SET_ISSI
	slot = create_cqe(cq, SET_ISSI, 0x00, &issi, 4, nullptr, SET_ISSI_OUTPUT_LENGTH, nullptr);
	ring_doorbell(&init_seg->dbv, &slot, 1);
	validate_cqe(cq, slot);
	debug_cmd(cq, slot);

	printf("\nISSI version: %d\n", issi);

	// QUERY_PAGES boot
	slot = create_cqe(cq, QUERY_PAGES, 0x1, nullptr, 0, nullptr, 16, nullptr);
	ring_doorbell(&init_seg->dbv, &slot, 1);
	validate_cqe(cq, slot);
	debug_cmd(cq, slot);
}

void teardown_hca(HCA::Init_Seg* init_seg) {
	using namespace CMD;

	l4_uint32_t slot;

	// TEARDOWN_HCA
	/*slot = create_cqe(cq, TEARDOWN_HCA, 0x00, nullptr, 0, nullptr, TEARDOWN_HCA_OUTPUT_LENGTH, nullptr);
	ring_doorbell(&init_seg->dbv, &slot, 1);
	validate_cqe(cq, slot);*/

	// DISABLE_HCA
	slot = create_cqe(cq, DISABLE_HCA, 0x00, nullptr, 0, nullptr, 8, nullptr);
	ring_doorbell(&init_seg->dbv, &slot, 1);
	validate_cqe(cq, slot);
	debug_cmd(cq, slot);
}

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

	dma_cap = Device::bind_dma_space_to_device(dev);
	MEM::DMA_MEM cq_mem = MEM::alloc_dma_mem(dma_cap, 4096);

	HCA::Init_Seg* init_seg = (HCA::Init_Seg*)bar0;

	init_hca(init_seg, cq_mem);
	teardown_hca(init_seg);


	main_srv.loop();

	return 0;
}
