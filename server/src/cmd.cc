#include <stdexcept>
#include <stdio.h>
#include <chrono>
#include <thread>
#include "cmd.h"
#include "device.h"

using namespace CMD;
using namespace Device;

void CMD::poll_ownership(volatile CQE* cqe) {
    using namespace std;
    using namespace std::chrono;

	l4_uint8_t res;
    auto start = high_resolution_clock::now();
    while ((res = ioread32be(&cqe->ctrl) & CQE_OWNERSHIP_MASK)) {
        auto now = high_resolution_clock::now();
        if (duration_cast<milliseconds>(now - start).count() >= CMD_TIMEOUT_MS)
            throw runtime_error("CMD Timeout");
        this_thread::sleep_for(milliseconds(20));
    }
}

void CMD::check_cqe_status(volatile CQE* cqe) {
    CQE_STATUS status = (CQE_STATUS)((ioread32be(&cqe->ctrl) & CQE_STATUS_MASK) >> CQE_STATUS_OFFSET);
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
    CMD_STATUS status = (CMD_STATUS)((ioread32be(&cod->status) & COD_STATUS_MASK) >> COD_STATUS_OFFSET);
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
        printf("COD Error Code: %x | opcode: 0x%x\n", status,
            ioread32be((&cod->status) - 4) >> 16);
        fflush(stdout);
        throw std::runtime_error("COD Error");
    }
}

void CMD::prepare_mbb(volatile MBB* mailbox_block, l4_uint64_t phys_addr, l4_uint32_t block_number) {
    iowrite32be(&mailbox_block->next_pointer_msb, (l4_uint32_t)(phys_addr >> 32));
    iowrite32be(&mailbox_block->next_pointer_lsb, (l4_uint32_t)phys_addr);
    iowrite32be(&mailbox_block->block_number, block_number);
    iowrite32be(&mailbox_block->ctrl, 0);
}

void CMD::tie_mail_together(MEM::DMA_MEM* mailbox, l4_uint32_t mailbox_counter) {
    int offset_in_bytes;
    MBB* mailbox_block;
    l4_uint64_t phys_addr;
    for (l4_uint32_t i = 0; i < mailbox_counter; i++) {
        /* make mailbox blocks into a list for the hw
           by adding next_pointer and block_number to each */
        offset_in_bytes = MAILBOX_ALIGN_SIZE * i;
        mailbox_block = (MBB*)((char*)mailbox->virt + offset_in_bytes);
        phys_addr = mailbox->phys + offset_in_bytes + MAILBOX_ALIGN_SIZE;
        prepare_mbb(mailbox_block, phys_addr, i);
    }
    /* configure the last block in the MBB list
       by setting next_pointer to 0 and block_number to mailbox_counter */
    offset_in_bytes = MAILBOX_ALIGN_SIZE * mailbox_counter;
    mailbox_block = (MBB*)((char*)mailbox->virt + offset_in_bytes);
    prepare_mbb(mailbox_block, 0, mailbox_counter);
}

/* Puts command input data after the first 16 bytes into mailbox blocks (MBB).
   MBBs are auto added as needed (this assumes enough DMA memory is provided!) */
void CMD::pack_mail(MEM::DMA_MEM* mailbox, l4_uint32_t* payload, l4_uint32_t length) {
    MBB* package = (MBB*)mailbox->virt;
    l4_uint32_t data_counter = 0, mailbox_counter = 0;
    int offset_in_bytes;
    for (l4_uint32_t i = 0; i < length; i++) {
        if (data_counter == 128) {
            /* start new mailbox block after mailbox_data field is full */
            data_counter = 0;
            mailbox_counter++;
            offset_in_bytes = MAILBOX_ALIGN_SIZE * mailbox_counter;
            package = (MBB*)((char*)mailbox->virt + offset_in_bytes);
        }
        //printf("pack stage: %.5d | data_counter: %.3d | mailbox_counter %.2d\n", i, data_counter, mailbox_counter);
        iowrite32be(&package->mailbox_data[data_counter], payload[i]);
        data_counter++;
    }
    tie_mail_together(mailbox, mailbox_counter);
    //TODO calculate mailbox block signatures
}

l4_uint32_t CMD::create_cqe(MEM::Queue<CMD::CQE>& cq, OPCODE opcode, l4_uint32_t op_mod,
    l4_uint32_t* payload, l4_uint32_t payload_length, MEM::DMA_MEM* input_mailbox,
    l4_uint32_t output_length, MEM::DMA_MEM* output_mailbox) {
    output_length = (output_length + 2) * 4;
    CQE* cqe = &cq.start[cq.head];
    reg32* reg = (reg32*)cqe;
    /* init all regs to 0 (might be unnecessary) */
    for (int i = 0; i < 16; i++) iowrite32be(&reg[i], 0);
    l4_uint32_t input_length = (payload_length + 2) * 4;
    iowrite32be(&cqe->type, CQE_TYPE_MASK & (0x7U << CQE_TYPE_OFFSET));
    iowrite32be(&cqe->input_length, input_length);
    if (input_length > 8) {
        /* first 8 bytes of additional input data are being put into the CQE */
        iowrite32be(&cqe->cid.data[0], payload[0]);
        if (input_length > 12) {
            iowrite32be(&cqe->cid.data[1], payload[1]);
            if (input_length > 16) {
                /* input length overflow into mailbox
                   packaging payload (without first 8 bytes) into input mailbox */
                pack_mail(input_mailbox, &payload[2], payload_length - 2);
                iowrite32be(&cqe->input_mailbox_msb, (l4_uint32_t)(input_mailbox->phys >> 32));
                iowrite32be(&cqe->input_mailbox_lsb, (l4_uint32_t)input_mailbox->phys & CQE_MAILBOX_MASK);
            }
        }
    }
    iowrite32be(&cqe->cid.opcode, CID_OPCODE_MASK & (opcode << CID_OPCODE_OFFSET));
    iowrite32be(&cqe->cid.op_mod, CID_OP_MOD_MASK & op_mod);
    if (output_length > 16) {
        /* setup MBB chain for the output mailbox */
        l4_uint32_t mbb_bytes = 128 * 4;
        l4_uint32_t omb_length = output_length - 16;
        l4_uint32_t mailbox_counter = 0;
        if (omb_length > mbb_bytes) {
            mailbox_counter = omb_length / mbb_bytes;
            if (!(omb_length % mbb_bytes)) mailbox_counter--;
        }
        tie_mail_together(output_mailbox, mailbox_counter);
        iowrite32be(&cqe->output_mailbox_msb, (l4_uint32_t)(output_mailbox->phys >> 32));
        iowrite32be(&cqe->output_mailbox_lsb, (l4_uint32_t)output_mailbox->phys & CQE_MAILBOX_MASK);
    }
    iowrite32be(&cqe->output_length, output_length);
    iowrite32be(&cqe->ctrl, 1);
    
    l4_uint32_t slot = MEM::enqueue(cq);
    return slot;
}

void CMD::unpack_mail(MEM::DMA_MEM* mailbox, l4_uint32_t* payload, l4_uint32_t length) {
    MBB* package = (MBB*)mailbox->virt;
    l4_uint32_t data_counter = 0, mailbox_counter = 0;
    int offset_in_bytes;
    for (l4_uint32_t i = 0; i < length; i++) {
        if (data_counter == 128) {
            /* start new mailbox block */
            data_counter = 0;
            mailbox_counter++;
            offset_in_bytes = MAILBOX_ALIGN_SIZE * mailbox_counter;
            package = (MBB*)((char*)mailbox->virt + offset_in_bytes);
        }
        payload[i] = ioread32be(&package->mailbox_data[data_counter]);
        data_counter++;
    }
}

void CMD::get_cmd_output(MEM::Queue<CMD::CQE>& cq, l4_uint32_t slot, MEM::DMA_MEM* mailbox, l4_uint32_t* output, l4_uint32_t output_length) {
    CQE* cqe = &cq.start[slot];
    if (!output_length) return;
    output[0] = ioread32be(&cqe->cod.output[0]);
    if (output_length > 1) {
        output[1] = ioread32be(&cqe->cod.output[1]);
        if (output_length > 2) unpack_mail(mailbox, &output[2], output_length - 2);
    }
}

void CMD::ring_doorbell(reg32* dbv, l4_uint32_t* slots, int count) {
    l4_uint32_t dbr = 0;
    for (int i = 0; i < count; i++)
        dbr += (1 << slots[i]);
    iowrite32be(dbv, dbr);
}

void CMD::validate_cqe(MEM::Queue<CMD::CQE>& cq, l4_uint32_t* slots, int count) {
    for (int i = 0; i < count; i++) {
        CQE* cqe = &cq.start[slots[i]];
        poll_ownership(cqe);
        check_cqe_status(cqe);
        check_cod_status(&cqe->cod);
    }
}
