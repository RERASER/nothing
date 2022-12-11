#include <windows.h>

#include <stdlib.h>

#include "amex/amex.h"

#include "board/sg-reader.h"

#include "cxbhook/config.h"
#include "cxbhook/revio.h"
#include "cxbhook/led.h"
#include "cxbhook/network.h"

#include "cxbio/cxbio.h"

#include "gfxhook/gfx.h"
#include "gfxhook/d3d9.h"

#include "hook/process.h"

#include "hooklib/serial.h"
#include "hooklib/spike.h"

#include "platform/platform.h"

#include "util/dprintf.h"

static HMODULE cxb_hook_mod;
static process_entry_t cxb_startup;
static struct cxb_hook_config cxb_hook_cfg;

static DWORD CALLBACK cxb_pre_startup(void)
{
    HMODULE d3dc;
    HMODULE dbghelp;
    HRESULT hr;

    dprintf("--- Begin cxb_pre_startup ---\n");

    /* Pin the D3D shader compiler. This makes startup much faster. */

    d3dc = LoadLibraryW(L"D3DCompiler_43.dll");

    if (d3dc != NULL) {
        dprintf("Pinned shader compiler, hMod=%p\n", d3dc);
    } else {
        dprintf("Failed to load shader compiler!\n");
    }

    /* Pin dbghelp so the path hooks apply to it. */

    dbghelp = LoadLibraryW(L"dbghelp.dll");

    if (dbghelp != NULL) {
        dprintf("Pinned debug helper library, hMod=%p\n", dbghelp);
    } else {
        dprintf("Failed to load debug helper library!\n");
    }

    /* Config load */

    cxb_hook_config_load(&cxb_hook_cfg, L".\\segatools.ini");

    /* Hook Win32 APIs */

    gfx_hook_init(&cxb_hook_cfg.gfx);
    gfx_d3d9_hook_init(&cxb_hook_cfg.gfx, cxb_hook_mod);
    serial_hook_init();

    /* Initialize emulation hooks */

    hr = platform_hook_init(
            &cxb_hook_cfg.platform,
            "SDCA",
            "AAV1",
            cxb_hook_mod);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = cxb_dll_init(&cxb_hook_cfg.dll, cxb_hook_mod);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = amex_hook_init(&cxb_hook_cfg.amex, NULL);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = sg_reader_hook_init(&cxb_hook_cfg.aime, 12, cxb_hook_mod);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = revio_hook_init(&cxb_hook_cfg.revio);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = network_hook_init(&cxb_hook_cfg.network);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = led_hook_init(&cxb_hook_cfg.led);

    if (FAILED(hr)) {
        goto fail;
    }

    /* Initialize debug helpers */

    spike_hook_init(L".\\segatools.ini");

    dprintf("---  End  cxb_pre_startup ---\n");

    /* Jump to EXE start address */

    return cxb_startup();

fail:
    ExitProcess(EXIT_FAILURE);
}

BOOL WINAPI DllMain(HMODULE mod, DWORD cause, void *ctx)
{
    HRESULT hr;

    if (cause != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    cxb_hook_mod = mod;

    hr = process_hijack_startup(cxb_pre_startup, &cxb_startup);

    if (!SUCCEEDED(hr)) {
        dprintf("Failed to hijack process startup: %x\n", (int) hr);
    }

    return SUCCEEDED(hr);
}
