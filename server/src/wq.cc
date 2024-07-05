#include <stdio.h>
#include <l4/sys/scheduler>
#include "interface.h"

void WQ_impl::setup_and_start(unsigned long qsize, L4::Cap<L4::Thread> caller,
        L4::Cap<WQ_if> *res) {
    WQ_impl work_queue;

    auto sp = l4_sched_param(2);
    sp.affinity = l4_sched_cpu_set(0, 0);
    if (l4_error(L4Re::Env::env()->scheduler()->run_thread(
            Pthread::L4::cap(pthread_self()), sp))) {
        return;
    }

    if (work_queue.mem_alloc(qsize) != 0) {
        printf("Failed to allocate WQ memory\n");
        return;
    }

    if (!work_queue.registry.registry()->register_obj(&work_queue).is_valid()) {
        printf("Failed to register WQ\n");
        return;
    }

    *res = work_queue.obj_cap();

    printf("Setup complete\n");

    /* Notify server that this wp is ready to receive requests */
    auto tag = l4_ipc_send(caller.cap(), l4_utcb(),
            l4_msgtag(0, 0, 0, 0), L4_IPC_NEVER);
    if (l4_msgtag_has_error(tag)) {
        printf("Failed to notify server\n");
        return;
    }

    try {
        work_queue.registry.loop();
    }
    catch (...) {
        printf("Shutting down work queue\n");

        /* Unregister server handler, clean up resources and then leave */
        work_queue.registry.registry()->unregister_obj(&work_queue);
        work_queue.cleanup();
    }
}

long WQ_impl::op_sub(WQ_if::Rights, l4_uint32_t a, l4_uint32_t b, l4_uint32_t &res) {
    res = a - b;
    return L4_EOK;
}

long WQ_impl::op_get_rq(WQ_if::Rights, L4::Ipc::Cap<L4Re::Dataspace> &rq) {
    rq = L4::Ipc::make_cap_rw(this->rq);
    return L4_EOK;
}

long WQ_impl::op_get_sq(WQ_if::Rights, L4::Ipc::Cap<L4Re::Dataspace> &rq) {
    rq = L4::Ipc::make_cap_rw(this->rq);
    return L4_EOK;
}

long WQ_impl::op_get_dbr(WQ_if::Rights, L4::Ipc::Cap<L4Re::Dataspace> &dbr) {
    dbr = L4::Ipc::make_cap_rw(this->dbr);
    return L4_EOK;
}

long WQ_impl::op_terminate(WQ_if::Rights) {
    throw std::exception {};
    return L4_EOK;
}

int WQ_impl::mem_alloc(unsigned long qsize) {
    /* Alloc capabilities for memory */
    rq = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
    if (! rq.is_valid()) {
        printf("Failed to allocate cap slot for receive queue\n");
        return 1;
    }

    /* Allocate dataspaces for memory */
    if (L4Re::Env::env()->mem_alloc()->alloc(qsize, rq) < 0) {
        printf("Failed to allocate receive queue dataspace\n");
        return 1;
    }

    /* Map dataspaces into address space */
    if (L4Re::Env::env()->rm()->attach(&rq_addr, qsize,
            L4Re::Rm::F::Search_addr | L4Re::Rm::F::RW,
            L4::Ipc::make_cap_rw(rq)) < 0) {
        printf("Failed to attach receive queue dataspace\n");
        return 1;
    }

    rq_size = qsize;

    printf("Memory allocated\n");
    return 0;
}

void WQ_impl::cleanup() {
    /* Unmap dataspaces from address space */
    if (L4Re::Env::env()->rm()->detach(rq_addr, NULL) < 0)
        printf("Failed to detach RQ dataspace!\n");
    
    /* Free dataspace capabilities */
    L4Re::Util::cap_alloc.free(rq, L4Re::This_task, L4_FP_ALL_SPACES);
}