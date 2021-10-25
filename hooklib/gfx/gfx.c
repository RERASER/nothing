#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "hook/table.h"

#include "hooklib/config.h"
#include "hooklib/gfx/gfx.h"

#include "util/dprintf.h"

typedef BOOL (WINAPI *ShowWindow_t)(HWND hWnd, int nCmdShow);

static BOOL WINAPI hook_ShowWindow(HWND hWnd, int nCmdShow);

static struct gfx_config gfx_config;
static ShowWindow_t next_ShowWindow;

static const struct hook_symbol gfx_hooks[] = {
    {
        .name = "ShowWindow",
        .patch = hook_ShowWindow,
        .link = (void **) &next_ShowWindow,
    },
};

void gfx_hook_init(const struct gfx_config *cfg, HINSTANCE self)
{
    assert(cfg != NULL);

    if (!cfg->enable) {
        return;
    }

    memcpy(&gfx_config, cfg, sizeof(*cfg));
    hook_table_apply(NULL, "user32.dll", gfx_hooks, _countof(gfx_hooks));
}

static BOOL WINAPI hook_ShowWindow(HWND hWnd, int nCmdShow)
{
    dprintf("Gfx: ShowWindow hook hit\n");

    if (!gfx_config.framed && nCmdShow == SW_RESTORE) {
        nCmdShow = SW_SHOW;
    }

    return next_ShowWindow(hWnd, nCmdShow);
}

HRESULT gfx_frame_window(HWND hwnd)
{
    HRESULT hr;
    DWORD error;
    LONG style;
    RECT rect;
    BOOL ok;

    SetLastError(ERROR_SUCCESS);
    style = GetWindowLongW(hwnd, GWL_STYLE);
    error = GetLastError();

    if (error != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(error);
        dprintf("Gfx: GetWindowLongPtrW(%p, GWL_STYLE) failed: %x\n",
                hwnd,
                (int) hr);

        return hr;
    }

    ok = GetClientRect(hwnd, &rect);

    if (!ok) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        dprintf("Gfx: GetClientRect(%p) failed: %x\n", hwnd, (int) hr);

        return hr;
    }

    style |= WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
    ok = AdjustWindowRect(&rect, style, FALSE);

    if (!ok) {
        /* come on... */
        hr = HRESULT_FROM_WIN32(GetLastError());
        dprintf("Gfx: AdjustWindowRect failed: %x\n", (int) hr);

        return hr;
    }

    /* This... always seems to set an error, even though it works? idk */
    SetWindowLongW(hwnd, GWL_STYLE, style);

    ok = SetWindowPos(
            hwnd,
            HWND_TOP,
            rect.left,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            SWP_FRAMECHANGED | SWP_NOMOVE);

    if (!ok) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        dprintf("Gfx: SetWindowPos(%p) failed: %x\n", hwnd, (int) hr);

        return hr;
    }

    return S_OK;
}
