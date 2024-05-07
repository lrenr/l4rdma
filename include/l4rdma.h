#pragma once

#include "l4/sys/__typeinfo.h"
#include "l4/sys/cxx/capability.h"
#include "l4/sys/kobject"
#include <l4/re/dataspace>
#include <l4/sys/cxx/ipc_iface>
#include <l4/sys/cxx/ipc_types>
#include <l4/sys/capability>

struct WQ_if : L4::Kobject_t<WQ_if, L4::Kobject> {
	L4_INLINE_RPC(long, sub, (l4_uint32_t a, l4_uint32_t b, l4_uint32_t *res));
	L4_INLINE_RPC(long, get_rq, (L4::Ipc::Out<L4::Cap<L4Re::Dataspace>> rq));
	L4_INLINE_RPC(long, get_sq, (L4::Ipc::Out<L4::Cap<L4Re::Dataspace>> sq));
	L4_INLINE_RPC(long, get_dbr, (L4::Ipc::Out<L4::Cap<L4Re::Dataspace>> dbr));
	L4_INLINE_RPC(long, terminate, (), L4::Ipc::Send_only);
	typedef L4::Typeid::Rpcs<sub_t, get_rq_t, get_sq_t, get_dbr_t, terminate_t> Rpcs;
};

struct CQ_if : L4::Kobject_t<CQ_if, L4::Kobject> {
	L4_INLINE_RPC(long, get_cq, (L4::Ipc::Out<L4::Cap<L4Re::Dataspace>> cq));
	L4_INLINE_RPC(long, terminate, (), L4::Ipc::Send_only);
	typedef L4::Typeid::Rpcs<get_cq_t, terminate_t> Rpcs;
};

struct L4RDMA : L4::Kobject_t<L4RDMA, L4::Kobject> {
	L4_INLINE_RPC(long, create_wq, (unsigned long qsize, L4::Ipc::Out<L4::Cap<WQ_if>> wq));
	L4_INLINE_RPC(long, create_cq, (unsigned long qsize, L4::Ipc::Out<L4::Cap<CQ_if>> cq));
	typedef L4::Typeid::Rpcs<create_wq_t, create_cq_t> Rpcs;
};
