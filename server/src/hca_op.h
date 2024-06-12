#pragma once

namespace cmd {

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

}