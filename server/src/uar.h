#pragma once

#include "mem.h"

namespace UAR {

cu32 UAR_EQN_MASK = 0xff000000;
cu32 UAR_EQN_OFFSET = 24;

#pragma pack(4)
struct Page {
    reg32 rsvd0[8];
    reg32 cmdsn_and_cmd_and_cq_ci;
    reg32 cq_n;
    reg32 rsvd1[6];
    reg32 eqn_arm_and_update_ci;
    reg32 rsvd2[1];
    reg32 eqn_and_update_ci;
};
#pragma pack()

struct UAR {
    l4_uint32_t index;
    Page* addr;
};

struct UAR_PD {
    UAR base;
};

struct UAR_BD {};

struct UAR_ED {
    UAR uar;
};

typedef PA::Pool<UAR_PD, UAR_BD, UAR_ED> UAR_Page_Pool;
typedef PA::Block<UAR_BD, UAR_ED> UAR_Page_Block;
typedef PA::Element<UAR_BD, UAR_ED> UAR_Page;

void alloc_block(UAR_Page_Pool* upp);

void free_block(UAR_Page_Pool* upp, UAR_Page_Block* upb);

inline UAR_Page* alloc_page(UAR_Page_Pool* upp) {
    return PA::alloc_page<UAR_PD, UAR_BD, UAR_ED>(upp);
}

inline void free_page(UAR_Page_Pool* upp, UAR_Page* up) {
    PA::free_page<UAR_PD, UAR_BD, UAR_ED>(upp, up);
}

inline void remove_block_from_pool(UAR_Page_Pool* upp, UAR_Page_Block* upb) {
    PA::remove_block_from_pool<UAR_PD, UAR_BD, UAR_ED>(upp, upb);
}

inline void add_block_to_pool(UAR_Page_Pool* upp, UAR_Page_Block* upb) {
    PA::add_block_to_pool<UAR_PD, UAR_BD, UAR_ED>(upp, upb);
}

inline UAR_Page_Block* create_block(UAR_Page_Pool* upp) {
    return PA::create_block<UAR_PD, UAR_BD, UAR_ED>(upp);
}

inline void destroy_block(UAR_Page_Block* upb) {
    PA::destroy_block<UAR_BD, UAR_ED>(upb);
}

}