#pragma once

#include <l4/re/env>
#include "mem.h"

namespace HCA {

enum OPCODE {
    QUERY_HCA_CAP       = 0x100,
    QUERY_ADAPTER       = 0x101,
    INIT_HCA            = 0x102,
    TEARDOWN_HCA        = 0x103,
    ENABLE_HCA          = 0x104,
    DISABLE_HCA         = 0x105,
    QUERY_PAGES         = 0x107,
    MANAGE_PAGES        = 0x108,
    SET_HCA_CAP         = 0x109,
    QUERY_ISSI          = 0x10a,
    SET_ISSI            = 0x10b,
    SET_DRIVER_VERSION  = 0x10d,
};

#pragma pack(4)
struct Init_Seg {
    volatile reg32 fw_rev;
    volatile reg32 cmdif_rev_fw_sub;
    volatile reg32 padding0[2];
    volatile reg32 cmdq_addr_msb;
    volatile reg32 cmdq_addr_lsb_info;
    volatile reg32 dbv;
    volatile reg32 padding1[120];
    volatile reg32 initializing;
};
#pragma pack()

}