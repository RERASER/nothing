#pragma once
#include <windows.h>
#include <stdbool.h>
#include <stdint.h>

struct cxb_io_config {
    uint8_t test;
    uint8_t service;
    uint8_t coin;
    uint8_t cancel;
    uint8_t up;
    uint8_t down;
};

void cxb_io_config_load(
        struct cxb_io_config *cfg,
        const wchar_t *filename);