#pragma once

#include <l4/re/env>
#include "mem.h"

namespace Event {

#pragma pack(4)
struct EQE {
    reg32 type = 0;
    reg32 rsvd[7];
    reg32 data[7];
    reg32 ctrl = 0;
};

struct EQC {
    reg32 status = 0;
    reg32 rsvd0[1];
    reg32 page_offset = 0;
    reg32 log_eq_size_and_uar = 0;
    reg32 rsvd1[1];
    reg32 intr = 0;
    reg32 log_page_size = 0;
    reg32 rsvd2[3];
    reg32 consumer_counter = 0;
    reg32 producer_counter = 0;
    reg32 rsvd3[4];
};
#pragma pack()

enum EVENT_TYPE {
    EVENT_TYPE_COMPLETION_EVENT                = 0x0,
    EVENT_TYPE_LAST_WQE_REACHED                = 0x13,
    EVENT_TYPE_CQ_ERROR                        = 0x4,
    EVENT_TYPE_INTERNAL_ERROR                  = 0x8,
    EVENT_TYPE_PORT_STATE_CHANGE               = 0x9,
    EVENT_TYPE_TEMP_WARNING_EVENT              = 0x17,
    EVENT_TYPE_COMMAND_INTERFACE_COMPLETION    = 0xa,
    EVENT_TYPE_PAGE_REQUEST                    = 0xb,
};

enum EQ_STATUS {
    EQ_STATUS_OK                 = 0x0,
    EQ_STATUS_EQ_WRITE_FAILURE   = 0xa,
};

void init_eq(MEM::Queue<EQE>& eq);

}