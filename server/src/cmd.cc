#include <stdexcept>
#include <stdio.h>
#include "cmd.h"
#include "device.h"

using namespace CMD;

void CMD::poll_ownership(volatile CQE* cqe) {
    int count = 0;
    while (l4_uint8_t res = Device::ioread32be(&cqe->ctrl) & CQE_OWNERSHIP_MASK) {
        if (count == 50) break;
        count++;
        printf("%x", res);
    }
    printf("\n");
}

void CMD::check_cqe_status(volatile CQE* cqe) {
    reg32 status = (Device::ioread32be(&cqe->ctrl) & CQE_STATUS_MASK) >> CQE_STATUS_OFFSET;
    if (!status) return;
    printf("CQE Error Code: %x\n", status);
    fflush(stdout);
    throw std::runtime_error("CQE Error");
}

void CMD::check_cod_status(volatile COD* cod) {
    reg32 status = (Device::ioread32be(&cod->status) & COD_STATUS_MASK) >> COD_STATUS_OFFSET;
    if (!status) return;
    printf("COD Error Code: %x\n", status);
    fflush(stdout);
    throw std::runtime_error("COD Error");
}

l4_uint32_t CMD::create_and_enqueue_cqe(volatile CQ &cq, HCA::OPCODE opcode, reg32 op_mod) {
    using namespace Device;
    CQE* cqe = &cq.start[cq.head];
    iowrite32be(&cqe->type, CQE_TYPE_MASK & (0x7U << CQE_TYPE_OFFSET));
    iowrite32be(&cqe->input_lenght, 8);
    iowrite32be(&cqe->cid.opcode, CID_OPCODE_MASK & (opcode << CID_OPCODE_OFFSET));
    iowrite32be(&cqe->cid.op_mod, CID_OP_MOD_MASK & op_mod);
    iowrite32be(&cqe->output_lenght, 16);
    iowrite32be(&cqe->ctrl, 1);
    l4_uint32_t slot = cq.head;
    cq.head++;
    if (cq.head >= cq.size)
        cq.head = 0;
    return slot;
}

void CMD::validate_cqe(volatile CQ &cq, l4_uint32_t slot) {
    CQE* cqe = &cq.start[slot];
    poll_ownership(cqe);
    check_cqe_status(cqe);
    check_cod_status(&cqe->cod);
}