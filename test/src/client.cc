#include "l4/sys/cxx/capability.h"
#include <stdio.h>
#include <l4/re/util/cap_alloc>
#include <l4/re/env>
#include <l4/l4rdma/l4rdma.h>

int main(int argc, char **argv) {
	const unsigned long QSIZE = 32;
	L4::Cap<L4RDMA> server;
	L4::Cap<CQ_if> cq;
	L4::Cap<WQ_if> wq;
	
	printf("Starting l4rdma test\n");

	server = L4Re::Env::env()->get_cap<L4RDMA>("l4rdma_server");
	if (!server.is_valid()) {
		printf("Could not get server capability!\n");
		return 1;
    }
	cq = L4Re::Util::cap_alloc.alloc<CQ_if>();
	if (!cq.is_valid()) {
		printf("Could not get CQ capability!\n");
		return 1;
    }
	wq = L4Re::Util::cap_alloc.alloc<WQ_if>();
	if (!wq.is_valid()) {
		printf("Could not get WQ capability!\n");
		return 1;
    }

	if (server->create_cq(QSIZE, cq) != L4_EOK) {
		printf("Failed to create CQ\n");
		return 1;
	}
	if (server->create_wq(QSIZE, wq) != L4_EOK) {
		printf("Failed to create WQ\n");
		return 1;
	}

	l4_uint32_t val1 = 8;
	l4_uint32_t val2 = 5;

	printf("Asking for %d - %d\n", val1, val2);
	if (wq->sub(val1, val2, &val1)) {
		printf("Error talking to server\n");
		return 1;
    }
	printf("Result of subtract call: %d\n", val1);

	L4::Cap<L4Re::Dataspace> rq;
    l4_uint8_t *rq_addr;

	rq = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();

	if (wq->get_rq(rq) != L4_EOK) {
		printf("Failed to get RQ\n");
		return 1;
	}
	
	printf("RQ Address: %d\n", rq_addr);

	if (L4Re::Env::env()->rm()->attach(&rq_addr, QSIZE,
			L4Re::Rm::F::Search_addr | L4Re::Rm::F::RW, rq) < 0) {
		printf("Failed to map rq dataspace\n");
		return 1;
	}

	printf("RQ Address: %d\n", rq_addr);

	if (wq->terminate() != L4_EOK) {
		printf("Failed to terminate WQ\n");
		return 1;
	}
	if (cq->terminate() != L4_EOK) {
		printf("Failed to terminate CQ\n");
		return 1;
	}

	if (server->kill() != L4_EOK) {
		printf("Failed to kill server\n");
		return 1;
	}

	printf("success\n");

	return 0;
}
