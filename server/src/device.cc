#include <stdio.h>
#include "device.h"

void Device::pci_list_dev(L4::Cap<L4vbus::Vbus> vbus)
{
    auto root = vbus->root();
    L4vbus::Pci_dev child;

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

    }
	printf("done\n\n");
}