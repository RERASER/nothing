#include <windows.h>

#include <assert.h>
#include <stdlib.h>

#include "cxbhook/cxb-dll.h"

#include "util/dll-bind.h"
#include "util/dprintf.h"

const struct dll_bind_sym cxb_dll_syms[] = {
    {
        .sym = "cxb_io_revio_init",
        .off = offsetof(struct cxb_dll, revio_init),
    },
    {
        .sym = "cxb_io_revio_poll",
        .off = offsetof(struct cxb_dll, revio_poll),
    },
    {
        .sym = "cxb_io_revio_get_coins",
        .off = offsetof(struct cxb_dll, revio_get_coins),
    },
    {
        .sym = "cxb_io_revio_set_coins",
        .off = offsetof(struct cxb_dll, revio_set_coins),
    },
    {
        .sym = "cxb_io_led_init",
        .off = offsetof(struct cxb_dll, led_init),
    },
    {
        .sym = "cxb_io_led_update",
        .off = offsetof(struct cxb_dll, led_update),
    },
};

struct cxb_dll cxb_dll;

// Copypasta DLL binding and diagnostic message boilerplate.
// Not much of this lends itself to being easily factored out. Also there
// will be a lot of API-specific branching code here eventually as new API
// versions get defined, so even though these functions all look the same
// now this won't remain the case forever.

HRESULT cxb_dll_init(const struct cxb_dll_config *cfg, HINSTANCE self)
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
            dprintf("Crossbeats IO: Failed to load IO DLL: %lx: %S\n",
                    hr,
                    cfg->path);

            goto end;
        }

        dprintf("Crossbeats IO: Using custom IO DLL: %S\n", cfg->path);
        src = owned;
    } else {
        owned = NULL;
        src = self;
    }

    get_api_version = (void *) GetProcAddress(src, "cxb_io_get_api_version");

    if (get_api_version != NULL) {
        cxb_dll.api_version = get_api_version();
    } else {
        cxb_dll.api_version = 0x0100;
        dprintf("Custom IO DLL does not expose cxb_io_get_api_version, "
                "assuming API version 1.0.\n"
                "Please ask the developer to update their DLL.\n");
    }

    if (cxb_dll.api_version >= 0x0200) {
        hr = E_NOTIMPL;
        dprintf("Crossbeats IO: Custom IO DLL implements an unsupported "
                "API version (%#04x). Please update Segatools.\n",
                cxb_dll.api_version);

        goto end;
    }

    sym = cxb_dll_syms;
    hr = dll_bind(&cxb_dll, src, &sym, _countof(cxb_dll_syms));

    if (FAILED(hr)) {
        if (src != self) {
            dprintf("Crossbeats IO: Custom IO DLL does not provide function "
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
