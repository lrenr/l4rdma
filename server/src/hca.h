#pragma once

#include <l4/re/env>
#include "mem.h"

namespace HCA {

#pragma pack(4)
struct Init_Seg {
    reg32 fw_rev;
    reg32 cmdif_rev_fw_sub;
    reg32 padding0[2];
    reg32 cmdq_addr_msb;
    reg32 cmdq_addr_lsb_info;
    reg32 dbv;
    reg32 padding1[120];
    reg32 initializing;
};
#pragma pack()

}