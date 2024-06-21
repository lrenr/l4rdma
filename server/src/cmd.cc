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
    CQE_STATUS status = (CQE_STATUS)((Device::ioread32be(&cqe->ctrl) & CQE_STATUS_MASK) >> CQE_STATUS_OFFSET);
    switch (status) {
    case CQE_STATUS_OK: return;
    case CQE_STATUS_BAD_SIG:
    case CQE_STATUS_BAD_TOKEN:
    case CQE_STATUS_BAD_BNUM:
    case CQE_STATUS_BAD_OUT_PTR:
    case CQE_STATUS_BAD_IN_PTR:
    case CQE_STATUS_INTERNAL_ERR:
    case CQE_STATUS_IN_LEN_ERR:
    case CQE_STATUS_OUT_LEN_ERR:
    case CQE_STATUS_PAD_NOT_ZERO:
    case CQE_STATUS_BAD_TYPE:
    default:
        printf("CQE Error Code: %x\n", status);
        fflush(stdout);
        throw std::runtime_error("CQE Error");
    }
}

void CMD::check_cod_status(volatile COD* cod) {
    CMD_STATUS status = (CMD_STATUS)((Device::ioread32be(&cod->status) & COD_STATUS_MASK) >> COD_STATUS_OFFSET);
    switch (status) {
    case CMD_STATUS_OK: return;
    case CMD_STATUS_INT_ERR:
    case CMD_STATUS_BAD_OP_ERR:
    case CMD_STATUS_BAD_PARAM_ERR:
    case CMD_STATUS_BAD_SYS_STATE_ERR:
    case CMD_STATUS_BAD_RES_ERR:
    case CMD_STATUS_RES_BUSY:
    case CMD_STATUS_LIM_ERR:
    case CMD_STATUS_BAD_RES_STATE_ERR:
    case CMD_STATUS_IX_ERR:
    case CMD_STATUS_NO_RES_ERR:
    case CMD_STATUS_BAD_INP_LEN_ERR:
    case CMD_STATUS_BAD_OUTP_LEN_ERR:
    case CMD_STATUS_BAD_QP_STATE_ERR:
    case CMD_STATUS_BAD_PKT_ERR:
    case CMD_STATUS_BAD_SIZE_OUTS_CQES_ERR:
    default:
        printf("COD Error Code: %x\n", status);
        fflush(stdout);
        throw std::runtime_error("COD Error");
    }
}

void CMD::package_mail(MEM::DMA_MEM* mailbox, l4_uint32_t* payload, l4_uint32_t length) {
    MBB* package = (MBB*)mailbox->virt;
    for (l4_uint32_t i = 0; i < length; i++) {
        Device::iowrite32be(&package->mailbox_data[i], payload[i]);
    }
    //TODO calculate mailbox block signatures
}

l4_uint32_t CMD::create_cqe(volatile CQ &cq, OPCODE opcode, l4_uint32_t op_mod,
    l4_uint32_t* payload, l4_uint32_t payload_length, MEM::DMA_MEM* input_mailbox,
    l4_uint32_t output_length, MEM::DMA_MEM* output_mailbox) {
    using namespace Device;

    CQE* cqe = &cq.start[cq.head];
    l4_uint32_t input_length = payload_length + 8;
    iowrite32be(&cqe->type, CQE_TYPE_MASK & (0x7U << CQE_TYPE_OFFSET));
    iowrite32be(&cqe->input_length, input_length);
    if (input_length > 8) {
        // first 8 bytes of additional input data are being put into the CQE
        iowrite32be(&cqe->cid.data[0], payload[0]);
        if (input_length > 12) {
            iowrite32be(&cqe->cid.data[1], payload[1]);
            if (input_length > 16) {
                // input length overflow into mailbox
                // packaging payload (without first 8 bytes) into input mailbox
                package_mail(input_mailbox, &payload[2], payload_length - 8);
                iowrite32be(&cqe->input_mailbox_msb, (l4_uint32_t)(input_mailbox->phys >> 32));
                iowrite32be(&cqe->input_mailbox_lsb, (l4_uint32_t)input_mailbox->phys & CQE_MAILBOX_MASK);
            }
        }
    }
    iowrite32be(&cqe->cid.opcode, CID_OPCODE_MASK & (opcode << CID_OPCODE_OFFSET));
    iowrite32be(&cqe->cid.op_mod, CID_OP_MOD_MASK & op_mod);
    if (output_length > 16) {
        // output length overflow into mailbox
        iowrite32be(&cqe->output_mailbox_msb, (l4_uint32_t)(output_mailbox->phys >> 32));
        iowrite32be(&cqe->output_mailbox_lsb, (l4_uint32_t)output_mailbox->phys & CQE_MAILBOX_MASK);
    }
    iowrite32be(&cqe->output_length, output_length);
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

void CMD::clear_mailbox(MEM::DMA_MEM &mailbox) {
    using namespace Device;
    MBB* mbb = (MBB*)mailbox.virt;
    for (int i = 0; i < 120; i++) iowrite32be(&mbb->mailbox_data[i], 0);
    iowrite32be(&mbb->next_pointer_msb, 0);
    iowrite32be(&mbb->next_pointer_lsb, 0);
    iowrite32be(&mbb->block_number, 0);
    iowrite32be(&mbb->ctrl, 0);
}

l4_uint32_t CMD::get_issi_support(MEM::DMA_MEM &mailbox) {
    reg32* data = (reg32*)mailbox.virt;
    l4_uint32_t issi;
    //TODO figure out max length
    for (int i = 0; i < 28; i++)
        if ((issi = Device::ioread32be(&data[i]))) break;
    clear_mailbox(mailbox);
    return issi;
}