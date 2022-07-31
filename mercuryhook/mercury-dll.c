#include <windows.h>

#include <assert.h>
#include <stdlib.h>

#include "mercuryhook/mercury-dll.h"

#include "util/dll-bind.h"
#include "util/dprintf.h"

const struct dll_bind_sym mercury_dll_syms[] = {
    {
        .sym = "mercury_io_init",
        .off = offsetof(struct mercury_dll, init),
    }, {
        .sym = "mercury_io_poll",
        .off = offsetof(struct mercury_dll, poll),
    }, {
        .sym = "mercury_io_get_opbtns",
        .off = offsetof(struct mercury_dll, get_opbtns),
    }, {
        .sym = "mercury_io_get_gamebtns",
        .off = offsetof(struct mercury_dll, get_gamebtns),
    }, {
        .sym = "mercury_io_touch_init",
        .off = offsetof(struct mercury_dll, touch_init),
    }, {
        .sym = "mercury_io_touch_start",
        .off = offsetof(struct mercury_dll, touch_start),
    }, {
        .sym = "mercury_io_touch_set_leds",
        .off = offsetof(struct mercury_dll, set_leds),
    }
};

struct mercury_dll mercury_dll;

// Copypasta DLL binding and diagnostic message boilerplate.
// Not much of this lends itself to being easily factored out. Also there
// will be a lot of API-specific branching code here eventually as new API
// versions get defined, so even though these functions all look the same
// now this won't remain the case forever.

HRESULT mercury_dll_init(const struct mercury_dll_config *cfg, HINSTANCE self)
{
    uint16_t (*get_api_version)(void);
    const struct dll_bind_sym *sym;
    HINSTANCE owned;
    HINSTANCE src;
    HRESULT hr;

    assert(cfg != NULL);
    assert(self != NULL);

    if (cfg->path[0] != L'\0') {
        owned = LoadLibraryW(cfg->path);

        if (owned == NULL) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            dprintf("Wacca IO: Failed to load IO DLL: %lx: %S\n",
                    hr,
                    cfg->path);

            goto end;
        }

        dprintf("Wacca IO: Using custom IO DLL: %S\n", cfg->path);
        src = owned;
    } else {
        owned = NULL;
        src = self;
    }

    get_api_version = (void *) GetProcAddress(src, "mercury_io_get_api_version");

    if (get_api_version != NULL) {
        mercury_dll.api_version = get_api_version();
    } else {
        mercury_dll.api_version = 0x0100;
        dprintf("Custom IO DLL does not expose mercury_io_get_api_version, "
                "assuming API version 1.0.\n"
                "Please ask the developer to update their DLL.\n");
    }

    if (mercury_dll.api_version >= 0x0200) {
        hr = E_NOTIMPL;
        dprintf("Wacca IO: Custom IO DLL implements an unsupported "
                "API version (%#04x). Please update Segatools.\n",
                mercury_dll.api_version);

        goto end;
    }

    sym = mercury_dll_syms;
    hr = dll_bind(&mercury_dll, src, &sym, _countof(mercury_dll_syms));

    if (FAILED(hr)) {
        if (src != self) {
            dprintf("Wacca IO: Custom IO DLL does not provide function "
                    "\"%s\". Please contact your IO DLL's developer for "
                    "further assistance.\n",
                    sym->sym);

            goto end;
        } else {
            dprintf("Internal error: could not reflect \"%s\"\n", sym->sym);
        }
    }

    owned = NULL;

end:
    if (owned != NULL) {
        FreeLibrary(owned);
    }

    return hr;
}
