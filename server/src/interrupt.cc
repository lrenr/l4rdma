#include <cstdio>
#include <l4/re/error_helper>
#include <l4/re/util/cap_alloc>
#include "interrupt.h"
#include "device.h"

void Interrupt::create_msix_entry(l4_uint32_t irq_num, volatile l4_uint32_t* msix_table, l4_icu_msi_info_t info) {
    using namespace Device;
    volatile l4_uint32_t* entry = msix_table + (irq_num * 4);
    iowrite32be(entry, (l4_uint32_t)info.msi_addr & 0xfffffffc);
    iowrite32be(entry + 1, (l4_uint32_t)(info.msi_addr >> 32));
    iowrite32be(entry + 2, info.msi_data);
    iowrite32be(entry + 3, 0);
}

L4::Cap<L4::Irq> Interrupt::create_and_bind_irq(l4_uint32_t irq_num, L4::Cap<L4::Icu> icu, L4::Cap<L4::Thread> thread) {
    L4::Cap<L4::Irq> irq = L4Re::chkcap(L4Re::Util::cap_alloc.alloc<L4::Irq>(),
        "Failed to allocated IRQ capability.");
    
    /* create IRQ object */
    L4Re::chksys(L4Re::Env::env()->factory()->create(irq));
    printf("04\n");
    fflush(stdout);

    /* bind IRQ to the current thread */
    L4Re::chksys(l4_error(irq->bind_thread(thread, 0)), 
        "Failed to bind IRQ to calling thread.");
    printf("05\n");
    fflush(stdout);
    
    /* bind IRQ to ICU */
    L4Re::chksys(l4_error(icu->bind(irq_num, irq)), 
        "Binding interrupt to ICU.");

    return irq;
}

L4::Cap<L4::Irq> Interrupt::create_msix_irq(l4_uint64_t icu_src, volatile l4_uint32_t* msix_table,
    l4_uint32_t irq_num, l4_uint32_t msix_vec_l4, L4::Cap<L4::Icu> icu) {

    L4::Cap<L4::Irq> irq = create_and_bind_irq(msix_vec_l4, icu, Pthread::L4::cap(pthread_self()));

    l4_icu_msi_info_t info;
    L4Re::chksys(icu->msi_info(msix_vec_l4, icu_src, &info), 
        "Failed to retrieve MSI info.");
    
    create_msix_entry(irq_num, msix_table, info);

    L4Re::chksys(l4_ipc_error(icu->unmask(msix_vec_l4), l4_utcb()),
        "Failed to unmask interrupt");

    return irq;
}

pthread_t Interrupt::create_msix_thread(l4_uint64_t icu_src, volatile l4_uint32_t* msix_table,
    l4_uint32_t irq_num, L4::Cap<L4::Icu> icu,
    void* (*handler_func)(void*), void* opt) {

    pthread_t handler_thread;
    l4_uint32_t msix_vec_l4 = irq_num | L4::Icu::F_msi;

    IRQH_Args args;
    args.irq_num = irq_num;
    args.msix_vec_l4 = msix_vec_l4;
    args.icu = icu;
    args.icu_src = icu_src;
    args.msix_table = msix_table;
    args.opt = opt;
    
    pthread_create(&handler_thread, NULL, handler_func, &args);

    return handler_thread;
}