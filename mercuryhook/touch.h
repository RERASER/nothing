#pragma once

#include <windows.h>

#include <stdbool.h>

struct touch_config {
    bool enable;
};

enum touch_cmd {
    CMD_GET_SYNC_BOARD_VER = 0xa0,
    CMD_STARTUP = 0x72,
    CMD_GET_UNIT_BOARD_VER = 0xa8,
    CMD_MYSTERY1 = 0xa2,
    CMD_MYSTERY2 = 0x94,
    CMD_START_AUTO_SCAN = 0xc9,
    CMD_BEGIN_WRITE = 0x77,
    CMD_NEXT_WRITE = 0x20
};

struct touch_req {
    uint8_t side; // COM3 or COM4
    uint8_t cmd; // First byte is the command byte
    uint8_t data[256]; // rest of the data goes here
    uint8_t data_length; // Size of the data including command byte
};

struct touch_input_frame {
    uint8_t cmd;
    uint8_t data1[24];
    uint8_t data2[9];
    uint8_t count;
    uint8_t checksum;
};

struct touch_resp_get_sync_board_ver {
    uint8_t cmd;
    char version[6];
    uint8_t checksum;
};

struct touch_resp_startup {
    char data[80];
    uint8_t checksum;
};

struct touch_resp_get_unit_board_ver {
    uint8_t cmd;
    uint8_t version[43];
    uint8_t checksum;
};

struct touch_resp_mystery1 {
    uint8_t cmd;
    uint8_t data;
    uint8_t checksum;
};

struct touch_resp_mystery2 {
    uint8_t cmd;
    uint8_t data;
    uint8_t checksum;
};

struct touch_resp_start_auto {
    uint8_t cmd;
    uint8_t data;
    uint8_t checksum;
    struct touch_input_frame frame;
};

HRESULT touch_hook_init(const struct touch_config *cfg);
