#pragma once

#include <l4/vbus/vbus>
#include <l4/vbus/vbus_pci>

namespace Device {

L4vbus::Pci_dev pci_get_dev(L4::Cap<L4vbus::Vbus> vbus);

l4_uint8_t* map_pci_bar(L4vbus::Pci_dev& dev, unsigned int idx);

static inline void set_reg32(l4_uint8_t* addr, int reg, l4_uint32_t value) {
    __asm__ volatile ("" : : : "memory");
    *((volatile l4_uint32_t*) (addr + reg)) = value;
}

static inline l4_uint32_t get_reg32(const l4_uint8_t* addr, int reg) {
    __asm__ volatile ("" : : : "memory");
    return *((volatile l4_uint32_t*) (addr + reg));
}

}