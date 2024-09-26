#include <stdexcept>
#include <stdio.h>
#include <thread>
#include <pthread-l4.h>
#include "interface.h"
#include "l4/sys/cxx/capability.h"

long L4RDMA_Server::op_create_wq(L4RDMA::Rights, unsigned long qsize, L4::Ipc::Cap<WQ_if> &wq) {
    L4::Cap<WQ_if> wq_if;

    /* Get thread cap to ourself (the WQ will report ready state to us via
       this cap) and launch handler thread */
    L4::Cap<L4::Thread> self = Pthread::L4::cap(pthread_self());
    std::thread wq_thread(WQ_impl::setup_and_start, qsize, self, &wq_if);
    /* Wait for the WQ thread to signal readiness */
    auto tag = l4_ipc_receive(pthread_l4_cap(wq_thread.native_handle()),
                            l4_utcb(), L4_IPC_NEVER);
    if (l4_msgtag_has_error(tag))
        printf("Failed to create WQ cap!\n");
    wq_thread.detach();

    wq = L4::Ipc::make_cap_rw(wq_if);
    printf("created WQ cap\n");
    return L4_EOK;
}

long L4RDMA_Server::op_create_cq(L4RDMA::Rights, unsigned long qsize, L4::Ipc::Cap<CQ_if> &cq) {
    L4::Cap<CQ_if> cq_if;

    /* Get thread cap to ourself (the CQ will report ready state to us via
       this cap) and launch handler thread */
    L4::Cap<L4::Thread> self = Pthread::L4::cap(pthread_self());
    std::thread cq_thread(CQ_impl::setup_and_start, qsize, self, &cq_if);
    /* Wait for the CQ thread to signal readiness */
    auto tag = l4_ipc_receive(pthread_l4_cap(cq_thread.native_handle()),
                            l4_utcb(), L4_IPC_NEVER);
    if (l4_msgtag_has_error(tag))
        printf("Failed to create CQ cap!\n");
    cq_thread.detach();

    cq = L4::Ipc::make_cap_rw(cq_if);
    printf("created CQ cap\n");
    return L4_EOK;
}

long L4RDMA_Server::op_kill(L4RDMA::Rights) {
    throw std::runtime_error("service loop killed");
}