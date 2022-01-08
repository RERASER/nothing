#pragma once

#include <windows.h>

#include "mercuryio/mercuryio.h"

struct mercury_dll {
    uint16_t api_version;
    HRESULT (*init)(void);
    HRESULT (*poll)(void);
    void (*get_opbtns)(uint8_t *opbtn);
    void (*get_gamebtns)(uint8_t *gamebtn);
    HRESULT (*touch_init)(void);
    void (*touch_start)(mercury_io_touch_callback_t callback);
};

struct mercury_dll_config {
    wchar_t path[MAX_PATH];
};

extern struct mercury_dll mercury_dll;

HRESULT mercury_dll_init(const struct mercury_dll_config *cfg, HINSTANCE self);
