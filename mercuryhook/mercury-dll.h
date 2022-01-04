#pragma once

#include <windows.h>

#include "mercuryio/mercuryio.h"

struct mercury_dll {
    uint16_t api_version;
    HRESULT (*init)(void);
    HRESULT (*poll)(void);
    void (*get_opbtns)(uint8_t *opbtn);
    void (*get_gamebtns)(uint16_t *player1, uint16_t *player2);
    HRESULT (*touch_init)(void);
};

struct mercury_dll_config {
    wchar_t path[MAX_PATH];
};

extern struct mercury_dll mercury_dll;

HRESULT mercury_dll_init(const struct mercury_dll_config *cfg, HINSTANCE self);
