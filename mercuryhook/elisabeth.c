#include <initguid.h>
#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "mercuryhook/elisabeth.h"

#include "hook/table.h"

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

void elisabeth_hook_init()
{
    dll_hook_insert_hooks(NULL);
    setupapi_add_phantom_dev(&elisabeth_guid, L"USB\\VID_0403&PID_6001");
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
