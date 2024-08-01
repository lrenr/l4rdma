#include <stdio.h>
#include <l4/re/env>
#include <l4/re/error_helper>
#include <l4/re/util/cap_alloc>
#include "device.h"
#include "l4/sys/cxx/capability.h"

L4vbus::Pci_dev Device::pci_get_dev(L4::Cap<L4vbus::Vbus> vbus)
{
    auto root = vbus->root();
    L4vbus::Pci_dev child, result;

    l4_uint8_t sclass_id, class_id;
    l4_uint16_t vendor_id, device_id;
    l4_uint32_t id_reg, cc_reg;

    printf("\nlisting PCI devices on vbus...\n");
    while (root.next_device(&child) == L4_EOK) {

        child.cfg_read(0, &id_reg, 32);
        child.cfg_read(8, &cc_reg, 32);

        vendor_id = (l4_uint16_t) id_reg;
        device_id = (l4_uint16_t) (id_reg >> 16);
        sclass_id = (l4_uint8_t) (cc_reg >> 16);
        class_id  = (l4_uint8_t) (cc_reg >> 24);
        printf("%.4x:%.4x CC_%.2x%.2x\n", vendor_id, device_id, class_id, sclass_id);
        if (vendor_id == 0x15b3)
            result = child;
    }
    printf("done\n\n");
    return result;
}

l4_uint8_t* Device::map_pci_bar(L4vbus::Pci_dev& dev, unsigned int idx) {

    l4_uint32_t lsb_addr, msb_addr, lsb_size, msb_size, cmd;
    l4_uint64_t bar_addr, bar_size;

    lsb_size = 0xffffffff;
    msb_size = 0xffffffff;

    printf("mapping pci bar...\n");

    /* disable mem & io spaces */
    L4Re::chksys(dev.cfg_read(0x04, &cmd, 32));
    cmd &= 0xfffffffc;
    cmd |= 1 << 2;
    L4Re::chksys(dev.cfg_write(0x04, cmd, 32));

    L4Re::chksys(dev.cfg_read(0x10 + idx * 4, &lsb_addr, 32));
    L4Re::chksys(dev.cfg_write(0x10 + idx * 4, lsb_size, 32));
    L4Re::chksys(dev.cfg_read(0x10 + idx * 4, &lsb_size, 32));
    L4Re::chksys(dev.cfg_write(0x10 + idx * 4, lsb_addr, 32));
    if (lsb_addr & 0x4) {
        /* 64-bit MEM-BAR */
        L4Re::chksys(dev.cfg_read(0x14 + idx * 4, &msb_addr, 32));
        L4Re::chksys(dev.cfg_write(0x14 + idx * 4, msb_size, 32));
        L4Re::chksys(dev.cfg_read(0x14 + idx * 4, &msb_size, 32));
        L4Re::chksys(dev.cfg_write(0x14 + idx * 4, msb_addr, 32));
        bar_addr = (l4_uint64_t) msb_addr << 32;
        bar_addr += lsb_addr & 0xfffffff0;
        bar_size = (l4_uint64_t) msb_size << 32;
        bar_size += lsb_size & 0xfffffff0;
        bar_size = ~bar_size + 1;
    } else if (lsb_addr & 0x1) {
        /* 32-bit IO-BAR */
        bar_addr = lsb_addr & 0xfffffffc;
        bar_size = (~(lsb_size & 0xfffffffc)) + 1;
    } else {
        /* 32-bit MEM-BAR */
        bar_addr = lsb_addr & 0xfffffff0;
        bar_size = (~(lsb_size & 0xfffffff0)) + 1;
    }

    /* reenable mem & io spaces */
    L4Re::chksys(dev.cfg_read(0x04, &cmd, 32));
    cmd &= 0xfffffffc;
    cmd += 0x00000003;
    L4Re::chksys(dev.cfg_write(0x04, cmd, 32));

    printf("addr: %.16llx\n", bar_addr);
    printf("size: %.16llx\n", bar_size);

    /* map BAR address space into local dataspace */
    l4_addr_t mem_addr;
    L4::Cap<L4Re::Dataspace> ds = L4::cap_reinterpret_cast<L4Re::Dataspace>(dev.bus_cap());
    L4Re::chksys(L4Re::Env::env()->rm()->attach(
        &mem_addr, l4_round_page(bar_size),
        L4Re::Rm::F::Search_addr | L4Re::Rm::F::Cache_uncached | L4Re::Rm::F::RW,
        L4::Ipc::make_cap_rw(ds), bar_addr, L4_PAGESHIFT));

    printf("done\n\n");

    return (l4_uint8_t *) mem_addr;
}

L4Re::Util::Shared_cap<L4Re::Dma_space> Device::bind_dma_space_to_device(L4vbus::Pci_dev& dev) {
    l4vbus_device_t devinfo;
	L4Re::chksys(dev.device(&devinfo));

	unsigned int i;
	l4vbus_resource_t res;
	for (i = 0; i < devinfo.num_resources; i++) {
		L4Re::chksys(dev.get_resource(i, &res));

		if (res.type == L4VBUS_RESOURCE_DMA_DOMAIN)
			break;
	}

	if (i == devinfo.num_resources)
		throw;

    L4Re::Util::Shared_cap<L4Re::Dma_space> dma_cap = L4Re::chkcap(L4Re::Util::make_shared_cap<L4Re::Dma_space>(), "Failed to allocate capability slot.");

	L4Re::chksys(L4Re::Env::env()->user_factory()->create(dma_cap.get()), "Failed to create DMA space.");

	// This is the step where we bind the DMA domain of the device to the
	// DMA space we just created for this task. The first argument to the
	// assignment operation is the domain_id, i.e. the starting address of
	// the DMA domain found in the set of device resources.
	// TODO: We could also choose the DMA domain of the whole vbus with ~0x0...
	L4Re::chksys(dev.bus_cap()->assign_dma_domain(res.start, L4VBUS_DMAD_BIND | L4VBUS_DMAD_L4RE_DMA_SPACE, dma_cap.get()), "Failed to bind to device's DMA domain.");

    return dma_cap;
}

l4_uint32_t Device::get_pci_cap(L4vbus::Pci_dev& dev, l4_uint8_t target_cap_id) {
    /* read status register */
    l4_uint32_t status;
    L4Re::chksys(dev.cfg_read(0x06, &status, 16));

    /* check if cap list exists (Bit 4) */
    if (!(status & 0x10)) return 0;

    l4_uint32_t cap_ptr = 0x34 - 1, cap_id;
    do {
        /* read capabilities pointer */
        L4Re::chksys(dev.cfg_read(cap_ptr + 1, &cap_ptr, 8));
        /* mask the lowest 2 bits */
        cap_ptr &= 0xfc;

        /* read capability id */
        L4Re::chksys(dev.cfg_read(cap_ptr, &cap_id, 8));
        if (cap_id == target_cap_id) return cap_ptr;
    } while (cap_ptr != 0);

    return 0;
}

void Device::create_icu_cap(L4vbus::Pci_dev& dev, L4::Cap<L4::Icu>& icu) {
    L4vbus::Icu vbus_icu;
    auto vbus = dev.bus_cap();
    if (vbus->root().device_by_hid(&vbus_icu, "L40009") < 0) throw;

    icu = L4Re::chkcap(L4Re::Util::cap_alloc.alloc<L4::Icu>(), 
        "Failed to allocate ICU capability");
    
    if (vbus_icu.vicu(icu) != 0) throw;
    
    l4_icu_info_t icu_info;
    if (l4_error(icu->info(&icu_info)) < 0) throw;
    printf("\nICU\nfeatures: %x | nr_irqs: %u | nr_msis: %u\n\n", 
        icu_info.features, icu_info.nr_irqs, icu_info.nr_msis);
}

int Device::setup_msix(L4vbus::Pci_dev& dev, l4_uint32_t& table_offset, l4_uint32_t& bir, l4_uint32_t& table_size) {
    l4_uint32_t msix_cap = get_pci_cap(dev, 0x11);
    if (msix_cap == 0) return 1;

    /* read MSI-X control register */
    l4_uint32_t msix_ctrl;
    L4Re::chksys(dev.cfg_read(msix_cap + 0x2, &msix_ctrl, 16));
    
    /* set enable Bit in the control register */
    msix_ctrl |= 0x8000;
    msix_ctrl &= 0xffffbfff;
    L4Re::chksys(dev.cfg_write(msix_cap + 0x2, msix_ctrl, 16));

    /* get Table Size */
    table_size = msix_ctrl & 0x7ff;

    /* read Table Offset and BIR */
    l4_uint32_t table_pos;
    L4Re::chksys(dev.cfg_read(msix_cap + 0x4, &table_pos, 32));
    table_offset = table_pos & 0xfff8;
    bir = table_pos & 0x7;

    printf("table_size: %d | bir: %d | table_offset: %d\n", table_size, bir, table_offset);
    fflush(stdout);

    if (table_size == 0) return 1;
    return 0;
}