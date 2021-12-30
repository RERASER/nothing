#include <initguid.h>
#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "mercuryhook/elisabeth.h"
#include "mercuryhook/mercury-dll.h"

#include "hook/table.h"

#include "hooklib/uart.h"
#include "hooklib/dll.h"
#include "hooklib/path.h"
#include "hooklib/setupapi.h"

#include "util/dprintf.h"

/* Hooks targeted DLLs dynamically loaded by elisabeth. */

static void dll_hook_insert_hooks(HMODULE target);
static HMODULE WINAPI my_LoadLibraryW(const wchar_t *name);
static HMODULE (WINAPI *next_LoadLibraryW)(const wchar_t *name);
static FARPROC WINAPI my_GetProcAddress(HMODULE hModule, const char *name);
static FARPROC (WINAPI *next_GetProcAddress)(HMODULE hModule, const char *name);
static HRESULT elisabeth_handle_irp(struct irp *irp);
static HRESULT elisabeth_handle_irp_locked(struct irp *irp);

static CRITICAL_SECTION elisabeth_lock;
static struct uart elisabeth_uart;
static uint8_t elisabeth_written_bytes[520];
static uint8_t elisabeth_readable_bytes[520];

static const struct hook_symbol win32_hooks[] = {
    {
        .name = "LoadLibraryW",
        .patch = my_LoadLibraryW,
        .link = (void **) &next_LoadLibraryW,
    },
    {
        .name = "GetProcAddress",
        .patch = my_GetProcAddress,
        .link = (void **) &next_GetProcAddress
    }
};

static const wchar_t *target_modules[] = {
    L"USBIntLED.DLL"
};

static const size_t target_modules_len = _countof(target_modules);

HRESULT elisabeth_hook_init()
{
    dll_hook_insert_hooks(NULL);
    setupapi_add_phantom_dev(&elisabeth_guid, L"$ftdi");

    InitializeCriticalSection(&elisabeth_lock);

    uart_init(&elisabeth_uart, 1);
    elisabeth_uart.written.bytes = elisabeth_written_bytes;
    elisabeth_uart.written.nbytes = sizeof(elisabeth_written_bytes);
    elisabeth_uart.readable.bytes = elisabeth_readable_bytes;
    elisabeth_uart.readable.nbytes = sizeof(elisabeth_readable_bytes);

    return iohook_push_handler(elisabeth_handle_irp);
}

static HRESULT elisabeth_handle_irp(struct irp *irp)
{
    HRESULT hr;

    assert(irp != NULL);

    if (!uart_match_irp(&elisabeth_uart, irp)) {
        return iohook_invoke_next(irp);
    }

    EnterCriticalSection(&elisabeth_lock);
    hr = elisabeth_handle_irp_locked(irp);
    LeaveCriticalSection(&elisabeth_lock);

    return hr;
}

static HRESULT elisabeth_handle_irp_locked(struct irp *irp)
{
    //union elisabeth_req_any req;
    struct iobuf req_iobuf;
    HRESULT hr;

    if (irp->op == IRP_OP_OPEN) {
        dprintf("Elisabeth: Starting backend\n");
        hr = mercury_dll.elisabeth_init();

        if (FAILED(hr)) {
            dprintf("Elisabeth: Backend error: %x\n", (int) hr);

            return hr;
        }
    }

    hr = uart_handle_irp(&elisabeth_uart, irp);

    if (FAILED(hr) || irp->op != IRP_OP_WRITE) {
        return hr;
    }

    for (;;) {

        //req_iobuf.bytes = req.bytes;
        //req_iobuf.nbytes = sizeof(req.bytes);
        //req_iobuf.pos = 0;

        /*hr = elisabeth_frame_decode(&req_iobuf, &elisabeth_uart.written);

        if (hr != S_OK) {
            if (FAILED(hr)) {
                dprintf("Elisabeth: Deframe error: %x\n", (int) hr);
            }

            return hr;
        }

        hr = elisabeth_req_dispatch(&req);

        if (FAILED(hr)) {
            dprintf("Elisabeth: Processing error: %x\n", (int) hr);
        }*/
    }
}

static void dll_hook_insert_hooks(HMODULE target)
{
    hook_table_apply(
            target,
            "kernel32.dll",
            win32_hooks,
            _countof(win32_hooks));
}

static HMODULE WINAPI my_LoadLibraryW(const wchar_t *name)
{
    const wchar_t *name_end;
    const wchar_t *target_module;
    bool already_loaded;
    HMODULE result;
    size_t name_len;
    size_t target_module_len;

    if (name == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);

        return NULL;
    }

    // Check if the module is already loaded
    already_loaded = GetModuleHandleW(name) != NULL;

    // Must call the next handler so the DLL reference count is incremented
    result = next_LoadLibraryW(name);

    if (!already_loaded && result != NULL) {
        name_len = wcslen(name);

        for (size_t i = 0; i < target_modules_len; i++) {
            target_module = target_modules[i];
            target_module_len = wcslen(target_module);

            // Check if the newly loaded library is at least the length of
            // the name of the target module
            if (name_len < target_module_len) {
                continue;
            }

            name_end = &name[name_len - target_module_len];

            // Check if the name of the newly loaded library is one of the
            // modules the path hooks should be injected into
            if (_wcsicmp(name_end, target_module) != 0) {
                continue;
            }

            dprintf("Elisabeth: Loaded %S\n", target_module);

            dll_hook_insert_hooks(result);
            setupapi_hook_insert_hooks(result);
        }
    }

    return result;
}

FARPROC WINAPI my_GetProcAddress(HMODULE hModule, const char *name)
{
    uintptr_t ordinal = (uintptr_t) name;

    FARPROC result = next_GetProcAddress(hModule, name);

    if (ordinal > 0xFFFF) {
        /* Import by name */
        //dprintf("Elisabeth: GetProcAddress %s is %p\n", name, result);
    }

    return result;
}
