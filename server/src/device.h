#pragma once

#include <l4/vbus/vbus>
#include <l4/vbus/vbus_pci>

namespace Device {

void pci_list_dev(L4::Cap<L4vbus::Vbus> vbus);

}