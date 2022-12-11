#pragma once

#include <windows.h>
#include <stdint.h>

#include "cxbio/cxbio.h"

struct cxb_dll {
    uint16_t api_version;
    HRESULT (*revio_init)(void);
    void (*revio_poll)(uint16_t *opbtn);
    void (*revio_get_coins)(long *coins);
    void (*revio_set_coins)(int coins);
    HRESULT (*led_init)(void);
    void (*led_update)(int id, int color);
};

struct cxb_dll_config {
    wchar_t path[MAX_PATH];
};

extern struct cxb_dll cxb_dll;

HRESULT cxb_dll_init(const struct cxb_dll_config *cfg, HINSTANCE self);
