#pragma once

#include <windows.h>

#include <stdbool.h>

struct gfx_config {
    bool enable;
    bool windowed;
    bool framed;
    int monitor;
};

void gfx_hook_init(const struct gfx_config *cfg, HINSTANCE self);

HRESULT gfx_frame_window(HWND hwnd);
