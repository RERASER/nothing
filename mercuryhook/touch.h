#pragma once

#include <windows.h>

#include <stdbool.h>

struct touch_config {
    bool enable;
};

HRESULT touch_hook_init(const struct touch_config *cfg);
