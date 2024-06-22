#include <stdio.h>
#include <stdexcept>
#include "hca.h"
#include "device.h"

using namespace CMD;
using namespace Device;

void HCA::debug_cmd(volatile CMD::CQ &cq, l4_uint32_t slot) {
	reg32* entry = (reg32*)&cq.start[slot];
	for (int i = 0; i < 16; i++) printf("%.8x\n", ioread32be(&entry[i]));
}

void HCA::init_wait(reg32* initializing) {
	l4_uint32_t init;
	while (true) {
		printf(".");
		init = ioread32be(initializing);
		if (!(init >> 31)) break;
	}
	printf("\n");
}

l4_uint32_t HCA::get_issi_support(MEM::DMA_MEM& mailbox) {
    reg32* data = (reg32*)mailbox.virt;
    l4_uint32_t issi;
    //TODO figure out max length
    for (int i = 0; i < 28; i++)
        if ((issi = ioread32be(&data[i]))) break;
    //clear_mailbox(&mailbox);
    return issi;
}

l4_uint32_t HCA::get_number_of_pages(volatile CQ &cq, l4_uint32_t slot) {
    CQE* cqe = &cq.start[slot];
    return ioread32be(&cqe->cod.output[1]);
}

void HCA::provide_pages(volatile CMD::CQ &cq, reg32* dbv, MEM::DMA_MEM& imb_mem, MEM::DMA_MEM* init_page_mem, l4_uint32_t init_page_count) {
    l4_uint32_t cmd_count = (init_page_count*2)/IMB_MAX_DATA;
	l4_uint32_t remainder;
	if ((remainder = (init_page_count*2)%IMB_MAX_DATA)) {
		cmd_count++;
	}

	for (l4_uint32_t i = 0; i < cmd_count; i++) {
		l4_uint32_t payload_page_count;
		if (i == cmd_count - 1) payload_page_count = remainder ? remainder/2 : IMB_MAX_DATA/2;
		else payload_page_count = IMB_MAX_DATA/2;
		
		l4_uint32_t init_page_payload[2 + (payload_page_count * 2)];
		init_page_payload[0] = 0;
		init_page_payload[1] = payload_page_count;
		for (l4_uint32_t i = 0; i < payload_page_count; i++) {
			l4_uint64_t phys = init_page_mem->phys + (i * HCA_PAGE_SIZE);
			init_page_payload[2 + (i * 2)] = (l4_uint32_t)(phys >> 32);
			init_page_payload[2 + (i * 2) + 1] = (l4_uint32_t)phys;
		}

		// MANAGE_PAGES init
		l4_uint32_t slot = create_cqe(cq, MANAGE_PAGES, 0x1, init_page_payload, 2 + (payload_page_count * 2), &imb_mem, 16, nullptr);
		ring_doorbell(dbv, &slot, 1);
		validate_cqe(cq, &slot, 1);
	}
}

void HCA::init_hca(CMD::CQ& cq, dma& dma_cap, MEM::HCA_PAGE_MEM& hca_page_mem, Init_Seg* init_seg, MEM::DMA_MEM& cq_mem, MEM::DMA_MEM& imb_mem, MEM::DMA_MEM& omb_mem) {
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

	// ENABLE_HCA
	slot = create_cqe(cq, ENABLE_HCA, 0x0, nullptr, 0, nullptr, ENABLE_HCA_OUTPUT_LENGTH, nullptr);
	printf("slot: %d\n", slot);
	ring_doorbell(&init_seg->dbv, &slot, 1);
	validate_cqe(cq, &slot, 1);

	// QUERY_ISSI
	slot = create_cqe(cq, QUERY_ISSI, 0x0, nullptr, 0, nullptr, QUERY_ISSI_OUTPUT_LENGTH, &omb_mem);
	ring_doorbell(&init_seg->dbv, &slot, 1);
	validate_cqe(cq, &slot, 1);

	// Setting ISSI to Version 1 if supported or else 0
	l4_uint32_t issi = get_issi_support(omb_mem);
	if (issi) issi = 1;
	else throw std::runtime_error("ISSI version 1 not supported by Hardware");

	// SET_ISSI
	slot = create_cqe(cq, SET_ISSI, 0x0, &issi, 1, nullptr, SET_ISSI_OUTPUT_LENGTH, nullptr);
	ring_doorbell(&init_seg->dbv, &slot, 1);
	validate_cqe(cq, &slot, 1);

	printf("\nISSI version: %d\n", issi);

	// QUERY_PAGES Boot
	slot = create_cqe(cq, QUERY_PAGES, 0x1, nullptr, 0, nullptr, 16, nullptr);
	ring_doorbell(&init_seg->dbv, &slot, 1);
	validate_cqe(cq, &slot, 1);

	// Provide Boot Pages
	l4_uint32_t boot_page_count = get_number_of_pages(cq, slot);
	printf("Boot Pages: %d\n", boot_page_count);
	MEM::DMA_MEM* boot_page_mem = &hca_page_mem.page_mem[hca_page_mem.page_mem_count];
	hca_page_mem.page_mem_count++;
	*boot_page_mem = MEM::alloc_dma_mem(dma_cap, HCA_PAGE_SIZE * boot_page_count);
	provide_pages(cq, &init_seg->dbv, imb_mem, boot_page_mem, boot_page_count);

	// QUERY_HCA_CAP
	l4_uint32_t payload = 0;
	slot = create_cqe(cq, QUERY_HCA_CAP, 0x0, &payload, 1, nullptr, 4112, &omb_mem);
	ring_doorbell(&init_seg->dbv, &slot, 1);
	validate_cqe(cq, &slot, 1);

	// QUERY_PAGES Init
	slot = create_cqe(cq, QUERY_PAGES, 0x2, nullptr, 0, nullptr, 16, nullptr);
	ring_doorbell(&init_seg->dbv, &slot, 1);
	validate_cqe(cq, &slot, 1);

	// Provide Init Pages
	l4_uint32_t init_page_count = get_number_of_pages(cq, slot);
	printf("Init Pages: %d\n", get_number_of_pages(cq, slot));
	MEM::DMA_MEM* init_page_mem = &hca_page_mem.page_mem[hca_page_mem.page_mem_count];
	hca_page_mem.page_mem_count++;
	*init_page_mem = MEM::alloc_dma_mem(dma_cap, HCA_PAGE_SIZE * init_page_count);
	provide_pages(cq, &init_seg->dbv, imb_mem, init_page_mem, init_page_count);

}

void HCA::teardown_hca(CMD::CQ& cq, HCA::Init_Seg* init_seg) {
	using namespace CMD;

	l4_uint32_t slot;

	// TEARDOWN_HCA
	/*slot = create_cqe(cq, TEARDOWN_HCA, 0x0, nullptr, 0, nullptr, TEARDOWN_HCA_OUTPUT_LENGTH, nullptr);
	ring_doorbell(&init_seg->dbv, &slot, 1);
	validate_cqe(cq, &slot, 1);*/

	// DISABLE_HCA
	slot = create_cqe(cq, DISABLE_HCA, 0x0, nullptr, 0, nullptr, 8, nullptr);
	ring_doorbell(&init_seg->dbv, &slot, 1);
	validate_cqe(cq, &slot, 1);
	debug_cmd(cq, slot);
}