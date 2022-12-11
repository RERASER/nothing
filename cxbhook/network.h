#pragma once

#include <windows.h>
#include <stdbool.h>
#include <stdint.h>

struct network_config {
    bool enable;
    bool disable_ssl;
    char title_server[PATH_MAX];
};

HRESULT network_hook_init(struct network_config *cfg);