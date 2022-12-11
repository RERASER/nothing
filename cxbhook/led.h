#pragma once

#include <windows.h>
#include <stdbool.h>
#include <stdint.h>

struct led_config {
    bool enable;
};

HRESULT led_hook_init(struct led_config *cfg);