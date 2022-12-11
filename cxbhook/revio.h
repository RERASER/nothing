#pragma once

#include <windows.h>
#include <stdbool.h>
#include <stdint.h>

struct revio_config {
    bool enable;
    uint8_t test;
    uint8_t service;
    uint8_t coin;
    uint8_t up;
    uint8_t down;
    uint8_t cancel;
};

HRESULT revio_hook_init(struct revio_config *cfg);