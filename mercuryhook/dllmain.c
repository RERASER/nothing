#include <windows.h>

#include "board/io4.h"
#include "board/sg-reader.h"
#include "board/vfd.h"

#include "hook/process.h"

#include "hooklib/serial.h"
#include "hooklib/spike.h"

#include "gfxhook/gfx.h"
#include "gfxhook/d3d11.h"

#include "mercuryhook/config.h"
#include "mercuryhook/io4.h"
#include "mercuryhook/mercury-dll.h"
#include "mercuryhook/elisabeth.h"
#include "mercuryhook/touch.h"

#include "platform/platform.h"

#include "util/dprintf.h"

static HMODULE mercury_hook_mod;
static process_entry_t mercury_startup;
static struct mercury_hook_config mercury_hook_cfg;

/* This hook is based on mu3hook, with leaked mercuryhook i/o codes. */

static DWORD CALLBACK mercury_pre_startup(void)
{
    HRESULT hr;

    dprintf("--- Begin mercury_pre_startup ---\n");

    /* Load config */

    mercury_hook_config_load(&mercury_hook_cfg, L".\\segatools.ini");

    /* Hook Win32 APIs */

    dvd_hook_init(&mercury_hook_cfg.dvd, mercury_hook_mod);
    serial_hook_init();

    gfx_hook_init(&mercury_hook_cfg.gfx);
    gfx_d3d11_hook_init(&mercury_hook_cfg.gfx, mercury_hook_mod);

    /* Initialize emulation hooks */

    hr = platform_hook_init(
            &mercury_hook_cfg.platform,
            "SDFE",
            "ACA1",
            mercury_hook_mod);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = sg_reader_hook_init(&mercury_hook_cfg.aime, 1, mercury_hook_mod);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = vfd_hook_init(2);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = mercury_dll_init(&mercury_hook_cfg.dll, mercury_hook_mod);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = mercury_io4_hook_init(&mercury_hook_cfg.io4);

    if (FAILED(hr)) {
        goto fail;
    }

    /* Start elisabeth Hooks for the LED and IO Board DLLs */
    elisabeth_hook_init();

    touch_hook_init(&mercury_hook_cfg.touch);

    /* Initialize debug helpers */

    spike_hook_init(L".\\segatools.ini");

    dprintf("---  End  mercury_pre_startup ---\n");

    /* Jump to EXE start address */

    return mercury_startup();

fail:
    ExitProcess(EXIT_FAILURE);
}

BOOL WINAPI DllMain(HMODULE mod, DWORD cause, void *ctx)
{
    HRESULT hr;

    if (cause != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    mercury_hook_mod = mod;

    hr = process_hijack_startup(mercury_pre_startup, &mercury_startup);

    if (!SUCCEEDED(hr)) {
        dprintf("Failed to hijack process startup: %x\n", (int) hr);
    }

    return SUCCEEDED(hr);
}
