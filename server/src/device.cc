#include <stdio.h>
#include <l4/re/env>
#include <l4/re/error_helper>
#include "device.h"

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
        if (vendor_id == 0x10de)
            result = child;
    }
	printf("done\n\n");
    return result;
}

l4_uint8_t* Device::map_pci_bar(L4vbus::Pci_dev& dev, unsigned int idx) {

	l4_uint32_t lsb_addr, msb_addr, lsb_size, msb_size;
	l4_uint64_t bar_addr, bar_size;

	lsb_size = ~0x00;
	msb_size = ~0x00;

    printf("mapping pci bar...\n");
	L4Re::chksys(dev.cfg_read(0x10 + idx * 4, &lsb_addr, 32));
    L4Re::chksys(dev.cfg_write(0x10 + idx * 4, lsb_size, 32));
    L4Re::chksys(dev.cfg_read(0x10 + idx * 4, &lsb_size, 32));
    L4Re::chksys(dev.cfg_write(0x10 + idx * 4, lsb_addr, 32));
    if (lsb_addr & 0x4) {
        // 64-bit MEM-BAR
	    L4Re::chksys(dev.cfg_read(0x14 + idx * 4, &msb_addr, 32));
        L4Re::chksys(dev.cfg_write(0x14 + idx * 4, msb_size, 32));
        L4Re::chksys(dev.cfg_read(0x14 + idx * 4, &msb_size, 32));
        L4Re::chksys(dev.cfg_write(0x14 + idx * 4, msb_addr, 32));
        bar_addr = (l4_uint64_t) msb_addr << 32;
        bar_addr += lsb_addr & 0xfffffff0;
        bar_size = (l4_uint64_t) msb_size << 32;
        bar_size += lsb_addr & 0xfffffff0;
        bar_size = ~bar_size + 1;
    } else if (lsb_addr & 0x1) {
        // 32-bit IO-BAR
        bar_addr = lsb_addr & 0xfffffffc;
        bar_size = (~(lsb_size & 0xfffffffc)) + 1;
    } else {
        // 32-bit MEM-BAR
        bar_addr = lsb_addr & 0xfffffff0;
        bar_size = (~(lsb_size & 0xfffffff0)) + 1;
    }

    printf("addr: %.16llx\n", bar_addr);
    printf("size: %.16llx\n", bar_size);

    // map BAR address space into local dataspace
	l4_addr_t mem_addr;
	L4::Cap<L4Re::Dataspace> ds = L4::cap_reinterpret_cast<L4Re::Dataspace>(dev.bus_cap());
    L4Re::chksys(L4Re::Env::env()->rm()->attach(
    	&mem_addr, l4_round_page(bar_size),
        L4Re::Rm::F::Search_addr | L4Re::Rm::F::Cache_uncached | L4Re::Rm::F::RW,
        L4::Ipc::make_cap_rw(ds), bar_addr, L4_PAGESHIFT));

    printf("done\n\n");

	return (l4_uint8_t *) mem_addr;
}