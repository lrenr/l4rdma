#include <stdio.h>
#include <cstring>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <pthread-l4.h>
#include "driver.h"
#include "device.h"
#include "mem.h"
#include "interrupt.h"
#include "event.h"

using namespace CMD;
using namespace Device;

void Driver::debug_cmd(MLX5_Context& ctx, l4_uint32_t slot) {
	reg32* entry = (reg32*)&((CQE*)ctx.cq.start)[slot];
	for (int i = 0; i < 16; i++) printf("%.8x\n", ioread32be(&entry[i]));
}

void Driver::init_wait(reg32* initializing) {
	using namespace std;
	using namespace std::chrono;

	l4_uint8_t init;
	auto start = high_resolution_clock::now();
	while ((init = ioread32be(initializing) >> 31)) {
		auto now = high_resolution_clock::now();
		if (duration_cast<milliseconds>(now - start).count() >= INIT_TIMEOUT_MS)
			throw runtime_error("Init Timeout");
		this_thread::sleep_for(milliseconds(20));
	}
}

l4_uint32_t Driver::get_issi_support(MLX5_Context& ctx, l4_uint32_t slot) {
	l4_uint32_t issi_out[QUERY_ISSI_OUTPUT_LENGTH];
	get_cmd_output(ctx, slot, issi_out, QUERY_ISSI_OUTPUT_LENGTH);

	l4_uint32_t start = 6, end = QUERY_ISSI_OUTPUT_LENGTH - 1, pos = 0, count = 0;
	for (l4_uint32_t i = end; i >= start; i--) {
		if ((pos = ffs(issi_out[i]))) break;
		count++;
	}
	l4_uint32_t issi = (count * 32) + pos;

	return issi;
}

l4_int32_t Driver::get_number_of_pages(MLX5_Context& ctx, l4_uint32_t slot) {
	l4_uint32_t pages_out[QUERY_ISSI_OUTPUT_LENGTH];
	get_cmd_output(ctx, slot, pages_out, QUERY_PAGES_OUTPUT_LENGTH);

	return pages_out[1];
}

void Driver::set_driver_version(MLX5_Context& ctx) {
	l4_uint32_t slot;

	l4_uint32_t driver[18];
	char version[24] = "l4re,mlx5,1.000.000000";
	memcpy(&driver[2], version, 23);
	printf("Driver Version: ");
	for (int i = 0; i < 22; i++) printf("%c", ((char*)&driver[2])[i]);
	printf("\n");

	/* SET_DRIVER_VERSION */
	slot = create_cqe(ctx, SET_DRIVER_VERSION, 0x0, driver, 18, SET_DRIVER_VERSION_OUTPUT_LENGTH);
	ring_doorbell(ctx.dbv, &slot, 1);
	validate_cqe(ctx.cq, &slot, 1);
}

l4_uint32_t Driver::configure_issi(MLX5_Context& ctx) {
	l4_uint32_t slot;

	/* QUERY_ISSI */
	slot = create_cqe(ctx, QUERY_ISSI, 0x0, nullptr, 0, QUERY_ISSI_OUTPUT_LENGTH);
	ring_doorbell(ctx.dbv, &slot, 1);
	validate_cqe(ctx.cq, &slot, 1);

	/* Setting ISSI to Version 1 if supported or else 0 */
	l4_uint32_t issi = get_issi_support(ctx, slot);
	printf("Supported ISSI version: %d\n", issi);
	if (issi) issi = 1;
	else throw std::runtime_error("ISSI version 1 not supported by Hardware");

	/* SET_ISSI */
	slot = create_cqe(ctx, SET_ISSI, 0x0, &issi, 1, SET_ISSI_OUTPUT_LENGTH);
	ring_doorbell(ctx.dbv, &slot, 1);
	validate_cqe(ctx.cq, &slot, 1);

	return issi;
}

bool Driver::configure_hca_cap(MLX5_Context& ctx) {
	l4_uint32_t slot;

	/* QUERY_HCA_CAP */
	l4_uint32_t payload = 0;
	slot = create_cqe(ctx, QUERY_HCA_CAP, 0x0, &payload, 1, QUERY_HCA_CAP_OUTPUT_LENGTH);
	ring_doorbell(ctx.dbv, &slot, 1);
	validate_cqe(ctx.cq, &slot, 1);

	l4_uint32_t hca_cap[QUERY_HCA_CAP_OUTPUT_LENGTH];
	get_cmd_output(ctx, slot, hca_cap, QUERY_HCA_CAP_OUTPUT_LENGTH);

	/* 0x1 = 128 byte Cache line size supported */
	l4_uint32_t cache_line_128byte = (hca_cap[(0x2c / 4) + 2] >> 27) & 0x1;
	if (cache_line_128byte) hca_cap[(0x2c / 4) + 2] = 1 << 27;

	/* 0x0 = disable cmd signatures for input and output
	   0x1 = enable cmd signature generation by hw for output
	   0x3 = enable cmd signature checks for input */
	l4_uint32_t cmdif_checksum = (hca_cap[(0x40 / 4) + 2] >> 14) & 0x3;
	if (cmdif_checksum >= 0x1) hca_cap[(0x40 / 4) + 2] = 1 << 14;

	/* base log2 max number of sq (0 = setting not supported) */
	l4_uint32_t log_max_sq = (hca_cap[(0x6c / 4) + 2] >> 16) & 0x1f;
	if (log_max_sq) hca_cap[(0x6c / 4) + 2] = log_max_sq << 16;
	l4_uint32_t max_sq = 1 << log_max_sq;

	/* 0x1 = exec SET_DRIVER_VERSION command */
	l4_uint32_t driver_version = (hca_cap[(0x4c / 4) + 2] >> 30) & 0x1;

	printf("cache_line_128: %d | cmdif_support: %x | max_num_sq: %d | set_driver: %d\n", cache_line_128byte, cmdif_checksum, max_sq, driver_version);

	/* SET_HCA_CAP */
	slot = create_cqe(ctx, SET_HCA_CAP, 0x0, hca_cap, QUERY_HCA_CAP_OUTPUT_LENGTH, SET_HCA_CAP_OUTPUT_LENGTH);
	ring_doorbell(ctx.dbv, &slot, 1);
	validate_cqe(ctx.cq, &slot, 1);

	return driver_version;
}

void Driver::provide_pages(MLX5_Context& ctx, l4_uint32_t page_count) {
	l4_uint32_t slot;

	l4_uint32_t cmd_count = (page_count*2)/IMB_MAX_DATA;
	l4_uint32_t remainder;
	if ((remainder = (page_count*2)%IMB_MAX_DATA)) {
		cmd_count++;
	}

	printf("page_count: %d | cmd_count: %d\n", page_count, cmd_count);

	l4_uint32_t payload_page_count;
	MEM::MEM_Page* mp;
	l4_uint64_t phys;
	l4_uint32_t page_payload[IMB_MAX_PAGE_PAYLOAD];
	l4_uint32_t payload_length;

	for (l4_uint32_t i = 0; i < cmd_count; i++) {
		if (i == cmd_count - 1) payload_page_count = remainder ? remainder/2 : IMB_MAX_DATA/2;
		else payload_page_count = IMB_MAX_DATA/2;

		printf("payload_page_count: %d\n", payload_page_count);
		payload_length = 2 + (payload_page_count * 2);
		page_payload[0] = 0;
		page_payload[1] = payload_page_count;
		for (l4_uint32_t j = 0; j < payload_page_count; j++) {
			mp = MEM::alloc_page(&ctx.mem_page_pool);
			phys = mp->data.phys;
			page_payload[2 + (j * 2)] = (l4_uint32_t)(phys >> 32);
			page_payload[2 + (j * 2) + 1] = (l4_uint32_t)phys;
		}

		/* MANAGE_PAGES */
		slot = create_cqe(ctx, MANAGE_PAGES, 0x1, page_payload, payload_length, 2);
		ring_doorbell(ctx.dbv, &slot, 1);
		validate_cqe(ctx.cq, &slot, 1);
	}
}

l4_int32_t Driver::provide_boot_pages(MLX5_Context& ctx) {
	l4_uint32_t slot;

	/* QUERY_PAGES Boot */
	slot = create_cqe(ctx, QUERY_PAGES, 0x1, nullptr, 0, QUERY_PAGES_OUTPUT_LENGTH);
	ring_doorbell(ctx.dbv, &slot, 1);
	validate_cqe(ctx.cq, &slot, 1);

	/* Provide Boot Pages */
	l4_int32_t boot_page_count = get_number_of_pages(ctx, slot);
	if (boot_page_count < 0) throw;

	provide_pages(ctx, boot_page_count);

	return boot_page_count;
}

l4_int32_t Driver::provide_init_pages(MLX5_Context& ctx) {
	l4_uint32_t slot;

	/* QUERY_PAGES Init */
	slot = create_cqe(ctx, QUERY_PAGES, 0x2, nullptr, 0, QUERY_PAGES_OUTPUT_LENGTH);
	ring_doorbell(ctx.dbv, &slot, 1);
	validate_cqe(ctx.cq, &slot, 1);

	/* Provide Init Pages */
	l4_int32_t init_page_count = get_number_of_pages(ctx, slot);
	if (init_page_count < 0) throw;

	provide_pages(ctx, init_page_count);

	return init_page_count;
}

l4_uint32_t Driver::reclaim_pages(MLX5_Context& ctx, l4_int32_t page_count) {
	l4_uint32_t slot;
	l4_int32_t result = 0;

	l4_int32_t reclaim_page_count;
	//l4_int32_t page_count = 64 * MBB_MAX_COUNT;
	l4_uint32_t payload[2];
	payload[0] = 0;
	payload[1] = page_count;
	l4_uint32_t output_length = 2 + (page_count * 2);
	CQE* cqe;
	l4_uint32_t output[OMB_MAX_DATA + 2];
	l4_uint64_t phys;
	//do {
		/* MANAGE_PAGES Reclaim */
		slot = create_cqe(ctx, MANAGE_PAGES, 0x2, payload, 2, output_length);
		ring_doorbell(ctx.dbv, &slot, 1);
		validate_cqe(ctx.cq, &slot, 1);

		cqe = &((CQE*)ctx.cq.start)[slot];
		reclaim_page_count = (l4_int32_t)ioread32be(&cqe->cod.output[0]);

		get_cmd_output(ctx, slot, output, output_length);
		for (l4_int32_t i = 0; i < reclaim_page_count; i++) {
			phys = (l4_uint64_t)output[2 + (i * 2)] << 32;
			phys |= (l4_uint64_t)output[2 + (i * 2) + 1];
			MEM::free_page(&ctx.mem_page_pool, phys);
		}

		printf("reclaiming: %d\n", reclaim_page_count);
		result += reclaim_page_count;
	//} while (reclaim_page_count);

	printf("page_pool_size: %llu | page_block_count: %llu | hash_map_size: %lu\n", ctx.mem_page_pool.size, ctx.mem_page_pool.block_count, ctx.mem_page_pool.data.index.size());

	return result;
}

l4_uint32_t Driver::reclaim_all_pages(MLX5_Context& ctx) {
	l4_int32_t result = 0;
	l4_int32_t reclaim_page_count;
	l4_int32_t page_count = 64 * MBB_MAX_COUNT;

	do {
		reclaim_page_count = reclaim_pages(ctx, page_count);
		result += reclaim_page_count;
	} while (reclaim_page_count);

	return result;
}

void Driver::init_queue_obj_pool(MLX5_Context& ctx) {
	ctx.event_queue_pool.max = 1024;
	ctx.event_queue_pool.size = 0;
	ctx.event_queue_pool.block_size = 64;
	ctx.event_queue_pool.block_count = 0;
	ctx.event_queue_pool.alloc_block = Q::alloc_block<Event::EQ_CTX>;
	ctx.event_queue_pool.free_block = Q::free_block;
	ctx.event_queue_pool.data.page_entry_count = Event::PAGE_EQE_COUNT;
	ctx.event_queue_pool.data.uar_page_pool = &ctx.uar_page_pool;
	ctx.event_queue_pool.start = nullptr;
}

void Driver::init_hca(MLX5_Context& ctx) {
	using namespace Device;
	using namespace CMD;

	printf("Initializing ...\n\n");

	/* read Firmware version */
	l4_uint32_t fw_rev = ioread32be(&ctx.init_seg->fw_rev);
	l4_uint32_t cmd_rev = ioread32be(&ctx.init_seg->cmdif_rev_fw_sub);
	l4_uint16_t fw_rev_major = fw_rev;
	l4_uint16_t fw_rev_minor = fw_rev >> 16;
	printf("Firmware Version: %.4x:%.4x %.4x:%.4x\n", fw_rev_major, fw_rev_minor,
		(l4_uint16_t)cmd_rev, (l4_uint16_t)(cmd_rev >> 16));

	init_wait(&ctx.init_seg->initializing);

	l4_uint32_t info = ioread32be(&ctx.init_seg->cmdq_addr_lsb_info) & 0xff;
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

	/* write Command Queue Address to Device */
	l4_uint64_t addr = ctx.cq.dma_mem.phys;
	l4_uint32_t addr_lsb = addr & (~0x3ff);
	l4_uint32_t addr_msb = (l4_uint64_t)addr >> 32;
	addr_lsb = addr;
	addr_msb = (l4_uint64_t)addr >> 32;
	iowrite32be(&ctx.init_seg->cmdq_addr_msb, addr_msb);
	iowrite32be(&ctx.init_seg->cmdq_addr_lsb_info, addr_lsb);
	l4_uint32_t probe_addr_msb = ioread32be(&ctx.init_seg->cmdq_addr_msb);
	l4_uint32_t probe_addr_lsb = ioread32be(&ctx.init_seg->cmdq_addr_lsb_info);
	printf("og: %.16llx\n", addr);
	printf("ref: %.8x:%.8x\n", addr_msb, addr_lsb);
	printf("addr_msb:addr_lsb: %.8x:%.8x\n", probe_addr_msb, probe_addr_lsb);

	/* wait for initialization */
	init_wait(&ctx.init_seg->initializing);

	printf("\nInit_Seg\n");
	printf("%.8x\n", ioread32be(&ctx.init_seg->fw_rev));
	printf("%.8x\n", ioread32be(&ctx.init_seg->cmdif_rev_fw_sub));
	printf("...\n");
	printf("%.8x\n", ioread32be(&ctx.init_seg->cmdq_addr_msb));
	printf("%.8x\n", ioread32be(&ctx.init_seg->cmdq_addr_lsb_info));
	printf("...\n");
	printf("%.8x\n", ioread32be(&ctx.init_seg->initializing));
	printf("\n");

	//TODO hardware health check

	/* INIT SEQUENCE */
	l4_uint32_t slot;

	/* ENABLE_HCA */
	slot = create_cqe(ctx, ENABLE_HCA, 0x0, nullptr, 0, ENABLE_HCA_OUTPUT_LENGTH);
	ring_doorbell(ctx.dbv, &slot, 1);
	validate_cqe(ctx.cq, &slot, 1);
	printf("ENABLE_HCA successful\n\n");

	printf("configuring ISSI ...\n");
	l4_uint32_t issi = configure_issi(ctx);
	printf("ISSI version: %d\n\n", issi);

	printf("providing Boot Pages ...\n");
	l4_int32_t boot_page_count = provide_boot_pages(ctx);
	printf("Boot Pages: %d\n\n", boot_page_count);

	printf("configuring HCA capabilities ...\n");
	bool set_driver = configure_hca_cap(ctx);
	printf("\n");

	printf("providing Init Pages ...\n");
	l4_int32_t init_page_count = provide_init_pages(ctx);
	printf("Init Pages: %d\n\n", init_page_count);

	/* INIT_HCA */
	slot = create_cqe(ctx, INIT_HCA, 0x0, nullptr, 0, INIT_HCA_OUTPUT_LENGTH);
	ring_doorbell(ctx.dbv, &slot, 1);
	validate_cqe(ctx.cq, &slot, 1);
	printf("INIT_HCA successful\n\n");

	printf("setting driver version ...\n");
	if (set_driver) set_driver_version(ctx);
	printf("\n");

	alloc_uar(ctx, (l4_uint8_t*)ctx.init_seg);

	init_queue_obj_pool(ctx);

	printf("Initialization complete\n\n");
}

void Driver::teardown_hca(MLX5_Context& ctx) {
	using namespace CMD;

	l4_uint32_t slot;

	printf("Tearing Down ...\n\n");

	//TODO dealloc UAR

	/* TEARDOWN_HCA */
	slot = create_cqe(ctx, TEARDOWN_HCA, 0x0, nullptr, 0, TEARDOWN_HCA_OUTPUT_LENGTH);
	ring_doorbell(ctx.dbv, &slot, 1);
	validate_cqe(ctx.cq, &slot, 1);
	printf("TEARDOWN_HCA successful\n\n");

	printf("reclaiming pages ...\n");
	l4_uint32_t reclaimed_pages = reclaim_all_pages(ctx);
	printf("Reclaimed Pages: %d\n\n", reclaimed_pages);

	/* DISABLE_HCA */
	slot = create_cqe(ctx, DISABLE_HCA, 0x0, nullptr, 0, DISABLE_HCA_OUTPUT_LENGTH);
	ring_doorbell(ctx.dbv, &slot, 1);
	validate_cqe(ctx.cq, &slot, 1);
	//debug_cmd(cq, slot);
	printf("DISABLE_HCA successful\n\n");

	printf("Teardown complete\n\n");
}

void Driver::alloc_uar(MLX5_Context& ctx, l4_uint8_t* bar0) {
	UAR::UAR uar;
	l4_uint32_t slot, output[2];

	/* ALLOC_UAR */
	slot = create_cqe(ctx, ALLOC_UAR, 0x0, nullptr, 0, 2);
	ring_doorbell(ctx.dbv, &slot, 1);
	validate_cqe(ctx.cq, &slot, 1);
	get_cmd_output(ctx, slot, output, 2);
	uar.index = output[0];
	printf("UAR: 0x%x %d\n", uar.index, uar.index);

	uar.addr = (UAR::Page*)(bar0 + (HCA_PAGE_SIZE * uar.index));

	ctx.uar_page_pool.max = 1024;
	ctx.uar_page_pool.size = 0;
	ctx.uar_page_pool.block_size = 64;
	ctx.uar_page_pool.block_count = 0;
	ctx.uar_page_pool.alloc_block = UAR::alloc_block;
	ctx.uar_page_pool.free_block = UAR::free_block;
	ctx.uar_page_pool.data.base.index = uar.index;
	ctx.uar_page_pool.data.base.addr = uar.addr;
	ctx.uar_page_pool.start = nullptr;
}

void* Driver::page_request_handler(void* arg) {
	using namespace Event;

	Interrupt::IRQH_Args args = *(Interrupt::IRQH_Args*)arg;
	PRH_OPT opt = *(PRH_OPT*)args.opt;
	l4_uint32_t eq = opt.eq;
	printf("msix_vec_l4: 0x%x\n", args.msix_vec_l4);
	fflush(stdout);

	L4::Cap<L4::Irq> irq = Interrupt::create_msix_irq(args.icu_src,
		args.msix_table, args.irq_num, args.msix_vec_l4, args.icu);

	while (opt.active) {
		irq->receive(L4_IPC_NEVER, l4_utcb());
		printf("interrupt!\n");
		fflush(stdout);
		if (eqe_owned_by_hw(*opt.ctx, eq)) continue;
		l4_uint32_t payload[7];
		read_eqe(*opt.ctx, eq, payload);
		printf("num_pages: %d\n", payload[1]);
		fflush(stdout);
	}

	irq->detach(l4_utcb());
	printf("detach!\n");

	pthread_exit(NULL);
}

void Driver::setup_event_queue(MLX5_Context& ctx, l4_uint64_t icu_src, reg32* msix_table, L4::Cap<L4::Icu>& icu) {
	using namespace Event;

	cu32 irq_num = 0;

	l4_uint32_t eq_number = create_eq(ctx, 4, EVENT_TYPE_PAGE_REQUEST, irq_num);

	l4_uint32_t state = get_eq_state(ctx, eq_number);
	printf("eq_state: 0x%x\n", state);

	PRH_OPT opt;
	opt.ctx = &ctx;
	opt.active = true;
	opt.eq = eq_number;
	pthread_t handler_thread = Interrupt::create_msix_thread(icu_src, msix_table, irq_num,
		icu, page_request_handler, &opt);

	arm_eq(ctx, eq_number);

	state = get_eq_state(ctx, eq_number);
	printf("eq_state: 0x%x\n", state);

	printf("before join\n");
	fflush(stdout);
	pthread_join(handler_thread, NULL);
	printf("after join\n");
}
