#pragma once

#include "l4/sys/cxx/capability.h"
#include "l4/sys/l4int.h"
#include <pthread-l4.h>
#include <l4/re/env>
#include <l4/re/util/object_registry>
#include <l4/re/util/br_manager>
#include <l4/re/dataspace>
#include <l4/sys/cxx/ipc_epiface>
#include <l4/l4rdma/l4rdma.h>

typedef L4Re::Util::Registry_server<L4Re::Util::Br_manager_hooks> Registry;

class WQ_impl : public L4::Epiface_t<WQ_impl, WQ_if> {
public:
	static void setup_and_start(unsigned long qsize, L4::Cap<L4::Thread> caller,
			L4::Cap<WQ_if> *res);
	long op_sub(WQ_if::Rights, l4_uint32_t a, l4_uint32_t b, l4_uint32_t &res);
	long op_get_rq(WQ_if::Rights, L4::Ipc::Cap<L4Re::Dataspace> &rq);
	long op_get_sq(WQ_if::Rights, L4::Ipc::Cap<L4Re::Dataspace> &rq);
	long op_get_dbr(WQ_if::Rights, L4::Ipc::Cap<L4Re::Dataspace> &dbr);
	long op_terminate(WQ_if::Rights);
protected:
	Registry registry{Pthread::L4::cap(pthread_self()),
			L4Re::Env::env()->factory()};
	L4::Cap<L4Re::Dataspace> rq;
	L4::Cap<L4Re::Dataspace> sq;
	L4::Cap<L4Re::Dataspace> dbr;
	l4_uint8_t *rq_addr;
	l4_uint8_t *sq_addr;
	l4_uint8_t *dbr_addr;
	l4_size_t rq_size;
	l4_size_t sq_size;
	l4_size_t dbr_size;
private:
	int mem_alloc(unsigned long qsize);
	void cleanup();
};

class CQ_impl : public L4::Epiface_t<CQ_impl, CQ_if> {
public:
	static void setup_and_start(unsigned long qsize, L4::Cap<L4::Thread> caller,
			L4::Cap<CQ_if> *res);
	long op_get_cq(CQ_if::Rights, L4::Ipc::Cap<L4Re::Dataspace> &cq);
	long op_terminate(CQ_if::Rights);
protected:
	Registry registry{Pthread::L4::cap(pthread_self()),
			L4Re::Env::env()->factory()};
	L4::Cap<L4Re::Dataspace> cq;
	l4_uint8_t *cq_addr;
	l4_size_t cq_size;
private:
	int mem_alloc(unsigned long qsize);
	void cleanup();
};

class L4RDMA_Server : public L4::Epiface_t<L4RDMA_Server, L4RDMA> {
public:
	long op_create_wq(L4RDMA::Rights, unsigned long qsize, L4::Ipc::Cap<WQ_if> &wq);
	long op_create_cq(L4RDMA::Rights, unsigned long qsize, L4::Ipc::Cap<CQ_if> &cq);
};