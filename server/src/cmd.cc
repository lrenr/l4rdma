#include <stdexcept>
#include <stdio.h>
#include "cmd.h"

using namespace cmd;

void cmd::poll_ownership(CQE &cqe) {
    while (!(cqe.ctrl & CQE_OWNERSHIP_MASK));
}

void cmd::check_cqe_status(CQE &cqe) {
    reg32 status = (cqe.ctrl & CQE_STATUS_MASK) >> CQE_STATUS_OFFSET;
    if (!status) return;
    printf("CQE Error Code: %x", status);
    throw std::runtime_error("CQE Error");
}

void cmd::check_cod_status(COD &cod) {
    reg32 status = (cod.status & COD_STATUS_MASK) >> COD_STATUS_OFFSET;
    if (!status) return;
    printf("COD Error Code: %x", status);
    throw std::runtime_error("COD Error");
}

CQE cmd::create_cqe(hca::OPCODE opcode, reg32 op_mod) {
    CID cid;
    cid.opcode = opcode;
    cid.op_mod = op_mod;
    COD cod;
    CQE cqe;
    cqe.type = CQE_TYPE_MASK & (0x7U << CQE_TYPE_OFFSET);
    cqe.input_lenght = 8;
    cqe.cid = cid;
    cqe.cod = cod;
    return cqe;
}

l4_uint32_t cmd::enqueue_cqe(CQE &cqe, CQ &cq) {
    cq.start[cq.head] = cqe;
    l4_uint32_t slot = cq.head;
    cq.head++;
    if (cq.head >= cq.size)
        cq.head = 0;
    return slot;
}

void cmd::validate_cqe(CQE &cqe) {
    poll_ownership(cqe);
    check_cqe_status(cqe);
    check_cod_status(cqe.cod);
}