#include <stdio.h>
#include "interface.h"
#include "device.h"
#include "hca.h"
#include "cmd.h"

cmd::CQ cq;

void ring_doorbell(l4_uint8_t* bar, l4_uint32_t slot) {
    l4_uint32_t dbr = (1 << slot);
    Device::set_reg32(bar, 0x18, dbr);
}

void init_hca(l4_uint8_t* bar) {
	// create default Command Queue
	cq.size = 8;
	cq.start = new cmd::CQE[cq.size];

	// read Firmware Version
    l4_uint32_t fw_rev = Device::get_reg32(bar, 0x00);
	l4_uint16_t fw_rev_major = fw_rev;
	l4_uint16_t fw_rev_minor = fw_rev >> 16;
	printf("Firmware Version: %.4x:%.4x", fw_rev_major, fw_rev_minor);

	// write Command Queue Address to Device
	l4_uint64_t addr = (l4_uint64_t)cq.start;
	l4_uint32_t addr_lsb = addr & (~0x3ff);
	l4_uint32_t addr_msb = addr >> 32;
    Device::set_reg32(bar, 0x10, addr_msb);
	Device::set_reg32(bar, 0x14, addr_lsb);

	// wait for initialization
	l4_uint32_t init;
	while (true) {
		init = Device::get_reg32(bar, 0x1fc);
		if (init & (1 << 31)) continue;
		break;
	}

	cmd::CQE cqe;
	l4_uint32_t slot;

	// ENABLE_HCA
	cqe = cmd::create_cqe(hca::ENABLE_HCA, 0x00);
	slot = cmd::enqueue_cqe(cqe, cq);
	ring_doorbell(bar, slot);
	cmd::validate_cqe(cqe);
}

void teardown_hca(l4_uint8_t* bar) {
	cmd::CQE cqe;
	l4_uint32_t slot;

	// TEARDOWN_HCA
	cqe = cmd::create_cqe(hca::TEARDOWN_HCA, 0x00);
	slot = cmd::enqueue_cqe(cqe, cq);
	ring_doorbell(bar, slot);
	cmd::validate_cqe(cqe);

	delete[] cq.start;
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

	init_hca(bar0);
	teardown_hca(bar0);


	main_srv.loop();

	return 0;
}
