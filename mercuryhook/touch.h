#pragma once

#include <windows.h>

#include <stdbool.h>

struct touch_config {
    bool enable;
};

enum touch_cmd {
    CMD_GET_REV_DATE = 0xa0,
    CMD_STARTUP = 0x72,
    CMD_GET_REV_DATE_DETAIL = 0xa8,
    CMD_UNKNOWN1 = 0xa2,
    CMD_UNKNOWN2 = 0x94,
    CMD_START_AUTO = 0xc9
};

struct touch_req {
    int side;
    int cmd; // First byte is the command byte
    int data[256]; // rest of the data goes here
    int data_length; // Size of the data including command byte
};

struct touch_input_frame {
    int data[36];
};

HRESULT touch_hook_init(const struct touch_config *cfg);
