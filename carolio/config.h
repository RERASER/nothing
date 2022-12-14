#pragma once

#include <stddef.h>
#include <stdint.h>

struct carol_io_config {
    uint8_t vk_buttons[5];
    uint8_t vk_slider[8];
    uint8_t vk_test;
    uint8_t vk_service;
    uint8_t vk_coin;
};

void carol_io_config_load(
        struct carol_io_config *cfg,
        const wchar_t *filename);
