#pragma once

#include <l4/vbus/vbus>
#include <l4/vbus/vbus_pci>

namespace Device {

L4vbus::Pci_dev pci_get_dev(L4::Cap<L4vbus::Vbus> vbus);

l4_uint8_t* map_pci_bar(L4vbus::Pci_dev& dev, unsigned int idx);

}