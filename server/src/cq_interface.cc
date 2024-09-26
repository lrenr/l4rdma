#include <stdio.h>
#include <l4/sys/scheduler>
#include "interface.h"

void CQ_impl::setup_and_start(unsigned long qsize, L4::Cap<L4::Thread> caller,
        L4::Cap<CQ_if> *res) {
    CQ_impl compl_queue;

    auto sp = l4_sched_param(2);
    sp.affinity = l4_sched_cpu_set(0, 0);
    if (l4_error(L4Re::Env::env()->scheduler()->run_thread(
            Pthread::L4::cap(pthread_self()), sp))) {
        return;
    }

    if (compl_queue.mem_alloc(qsize) != 0) {
        printf("Failed to allocate CQ memory\n");
        return;
    }

    if (!compl_queue.registry.registry()->register_obj(&compl_queue).is_valid()) {
        printf("Failed to register CQ\n");
        return;
    }

    *res = compl_queue.obj_cap();

    printf("Setup complete\n");

    /* Notify server that this cp is ready to receive requests */
    auto tag = l4_ipc_send(caller.cap(), l4_utcb(),
            l4_msgtag(0, 0, 0, 0), L4_IPC_NEVER);
    if (l4_msgtag_has_error(tag)) {
        printf("Failed to notify server\n");
        return;
    }

    try {
        compl_queue.registry.loop();
    }
    catch (...) {
        printf("Shutting down completion queue\n");

        /* Unregister server handler, clean up resources and then leave */
        compl_queue.registry.registry()->unregister_obj(&compl_queue);
        compl_queue.cleanup();
    }
}

long CQ_impl::op_get_cq(CQ_if::Rights, L4::Ipc::Cap<L4Re::Dataspace> &cq) {
    cq = L4::Ipc::make_cap_rw(this->cq);
    return L4_EOK;
}

long CQ_impl::op_terminate(CQ_if::Rights) {
    throw std::exception {};
    return L4_EOK;
}

int CQ_impl::mem_alloc(unsigned long qsize) {
    /* Alloc capabilities for memory */
    cq = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
    if (! cq.is_valid()) {
        printf("Failed to allocate cap slot for receive queue\n");
        return 1;
    }

    /* Allocate dataspaces for memory */
    if (L4Re::Env::env()->mem_alloc()->alloc(qsize, cq) < 0) {
        printf("Failed to allocate receive queue dataspace\n");
        return 1;
    }

    /* Map dataspaces into address space */
    if (L4Re::Env::env()->rm()->attach(&cq_addr, qsize,
            L4Re::Rm::F::Search_addr | L4Re::Rm::F::RW,
            L4::Ipc::make_cap_rw(cq)) < 0) {
        printf("Failed to attach receive queue dataspace\n");
        return 1;
    }

    cq_size = qsize;

    printf("Memory allocated\n");
    return 0;
}

void CQ_impl::cleanup() {
    /* Unmap dataspaces from address space */
    if (L4Re::Env::env()->rm()->detach(cq_addr, NULL) < 0)
        printf("Failed to detach RQ dataspace!\n");
    
    /* Free dataspace capabilities */
    L4Re::Util::cap_alloc.free(cq, L4Re::This_task, L4_FP_ALL_SPACES);
}