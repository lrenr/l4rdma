#pragma once

#include <l4/re/env>
#include <l4/vbus/vbus>
#include <l4/vbus/vbus_pci>
#include <pthread-l4.h>

namespace Interrupt {

struct IRQH_Args {
    l4_uint32_t irq_num;
    l4_uint32_t msix_vec_l4;
    L4::Cap<L4::Icu> icu;
    l4_uint64_t icu_src;
    volatile l4_uint32_t* msix_table;
    void* opt;
};

void create_msix_entry(l4_uint32_t irq_num, volatile l4_uint32_t* msix_table, l4_icu_msi_info_t info);

L4::Cap<L4::Irq> create_and_bind_irq(l4_uint32_t irq_num, L4::Cap<L4::Icu> icu, L4::Cap<L4::Thread> thread);

L4::Cap<L4::Irq> create_msix_irq(l4_uint64_t icu_src, volatile l4_uint32_t* msix_table,
    l4_uint32_t irq_num, l4_uint32_t msix_vec_l4, L4::Cap<L4::Icu> icu);

pthread_t create_msix_thread(l4_uint64_t icu_src, volatile l4_uint32_t* msix_table,
    l4_uint32_t irq_num, L4::Cap<L4::Icu> icu,
    void* (*handler_func)(void*), void* opt);

}