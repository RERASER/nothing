#include <windows.h>
#include <shlwapi.h>

#include <stdbool.h>
#include <stdlib.h>
#include <wchar.h>

#include "amex/amex.h"

#include "board/sg-reader.h"

#include "gfxhook/d3d11.h"
#include "gfxhook/dxgi.h"
#include "gfxhook/gfx.h"

#include "hook/process.h"

#include "hooklib/dvd.h"
#include "hooklib/serial.h"
#include "hooklib/spike.h"

#include "idzhook/config.h"
#include "idzhook/idz-dll.h"
#include "idzhook/jvs.h"
#include "idzhook/zinput.h"

#include "platform/platform.h"

#include "util/dprintf.h"
#include "util/lib.h"

static HMODULE idz_hook_mod;
static process_entry_t idz_startup;
static struct idz_hook_config idz_hook_cfg;

static DWORD CALLBACK idz_pre_startup(void)
{
    wchar_t *module_path;
    wchar_t *file_name;
    HRESULT hr;

    dprintf("--- Begin idz_pre_startup ---\n");

    /* Config load */

    idz_hook_config_load(&idz_hook_cfg, L".\\segatools.ini");

    module_path = module_file_name(NULL);

    if (module_path != NULL) {
        file_name = PathFindFileNameW(module_path);

        _wcslwr(file_name);

        if (wcsstr(file_name, L"serverbox") != NULL) {
            dprintf("Executable filename contains 'ServerBox', disabling full-screen mode\n");

            idz_hook_cfg.gfx.windowed = true;
            idz_hook_cfg.gfx.framed = true;
        }

        free(module_path);

        module_path = NULL;
    }

    /* Hook Win32 APIs */

    serial_hook_init();
    gfx_hook_init(&idz_hook_cfg.gfx, idz_hook_mod);
    gfx_d3d11_hook_init(&idz_hook_cfg.gfx, idz_hook_mod);
    gfx_dxgi_hook_init(&idz_hook_cfg.gfx, idz_hook_mod);
    zinput_hook_init(&idz_hook_cfg.zinput);
    dvd_hook_init(&idz_hook_cfg.dvd, idz_hook_mod);

    /* Initialize emulation hooks */

    hr = platform_hook_init(
            &idz_hook_cfg.platform,
            "SDDF",
            "AAV2",
            idz_hook_mod);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = idz_dll_init(&idz_hook_cfg.dll, idz_hook_mod);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = amex_hook_init(&idz_hook_cfg.amex, idz_jvs_init);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = sg_reader_hook_init(&idz_hook_cfg.aime, 10, idz_hook_mod);

    if (FAILED(hr)) {
        goto fail;
    }

    /* Initialize debug helpers */

    spike_hook_init(L".\\segatools.ini");

    dprintf("---  End  idz_pre_startup ---\n");

    /* Jump to EXE start address */

    return idz_startup();

fail:
    ExitProcess(EXIT_FAILURE);
}

BOOL WINAPI DllMain(HMODULE mod, DWORD cause, void *ctx)
{
    HRESULT hr;

    if (cause != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    idz_hook_mod = mod;

    hr = process_hijack_startup(idz_pre_startup, &idz_startup);

    if (!SUCCEEDED(hr)) {
        dprintf("Failed to hijack process startup: %x\n", (int) hr);
    }

    return SUCCEEDED(hr);
}
