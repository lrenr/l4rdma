#pragma once

#include <l4/vbus/vbus>
#include <l4/vbus/vbus_pci>
#include <l4/re/util/shared_cap>

namespace Device {

template <typename T>
T swap_endian(T u)
{
    union
    {
        T u;
        unsigned char u8[sizeof(T)];
    } source, dest;

    source.u = u;

    for (l4_size_t k = 0; k < sizeof(T); k++)
        dest.u8[k] = source.u8[sizeof(T) - k - 1];

    return dest.u;
}

L4vbus::Pci_dev pci_get_dev(L4::Cap<L4vbus::Vbus> vbus);

l4_uint8_t* map_pci_bar(L4vbus::Pci_dev& dev, unsigned int idx);

L4Re::Util::Shared_cap<L4Re::Dma_space> bind_dma_space_to_device(L4vbus::Pci_dev& dev);

l4_uint32_t get_pci_cap(L4vbus::Pci_dev& dev, l4_uint8_t target_cap_id);

void create_icu_cap(L4vbus::Pci_dev& dev, L4::Cap<L4::Icu>& icu);

int setup_msix(L4vbus::Pci_dev& dev, l4_uint32_t& table_offset, l4_uint32_t& bir, l4_uint32_t& table_size);

static inline void iowrite32be(volatile l4_uint32_t* addr, l4_uint32_t value) {
    __asm__ volatile ("" : : : "memory");
    *addr = swap_endian<l4_uint32_t>(value);
}

static inline l4_uint32_t ioread32be(volatile l4_uint32_t* addr) {
    __asm__ volatile ("" : : : "memory");
    return swap_endian<l4_uint32_t>(*addr);
}

static inline void iowrite32(volatile l4_uint32_t* addr, l4_uint32_t value) {
    __asm__ volatile ("" : : : "memory");
    *addr = value;
}

static inline l4_uint32_t ioread32(volatile l4_uint32_t* addr) {
    __asm__ volatile ("" : : : "memory");
    return *addr;
}

}