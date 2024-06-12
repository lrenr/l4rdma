#include <stdexcept>
#include <stdio.h>
#include "cmd.h"

using namespace cmd;

CQE create_cqe_exmail(CID cid) {
    COD cod;
    cod.status = 0;
    cod.syndrome = 0;
    cod.output = {0, 0};

    CQE cqe;
    cqe.type = CQE_TYPE_MASK & (0x7U << CQE_TYPE_OFFSET);
    cqe.input_lenght = 8;
    cqe.input_mailbox = 0;
    cqe.input_inline = cid;
    cqe.output_inline = cod;
    cqe.output_mailbox = 0;
    cqe.output_lenght = 0;
    cqe.ctrl = 0;

    return cqe;
}

bool poll_ownership(CQE &cqe) {
    while (!(cqe.ctrl & CQE_OWNERSHIP_MASK));
    return true;
}

void check_cqe_status(CQE &cqe) {
    l4_uint32_t status = (cqe.ctrl & CQE_STATUS_MASK) >> CQE_STATUS_OFFSET;
    if (!status) return;
    printf("CQE Error Code: %x", status);
    throw std::runtime_error("CQE Error");
}

void check_cod_status(COD &cod) {
    l4_uint32_t status = (cod.status & COD_STATUS_MASK) >> COD_STATUS_OFFSET;
    if (!status) return;
    printf("COD Error Code: %x", status);
    throw std::runtime_error("COD Error");
}