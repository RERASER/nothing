#pragma once

#include <windows.h>

void gfx_util_ensure_win_visible(HWND hwnd);
void gfx_util_borderless_fullscreen_windowed(HWND hwnd, LONG width, LONG height);
HRESULT gfx_util_frame_window(HWND hwnd);
