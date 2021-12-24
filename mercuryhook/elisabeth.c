#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "mercuryhook/elisabeth.h"

#include "hook/table.h"

#include "hooklib/dll.h"
#include "hooklib/path.h"

#include "util/dprintf.h"

/* Hooks targeted DLLs dynamically loaded by elisabeth. */

static void dll_hook_insert_hooks(HMODULE target);
static HMODULE WINAPI my_LoadLibraryW(const wchar_t *name);
static HMODULE (WINAPI *next_LoadLibraryW)(const wchar_t *name);
static FARPROC WINAPI my_GetProcAddress(HMODULE hModule, const char *name);
static FARPROC (WINAPI *next_GetProcAddress)(HMODULE hModule, const char *name);

static const struct hook_symbol elisabeth_hooks[] = {
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
    L"USBIntLED.DLL",
    L"ftd2XX.dll",
};

static const char *target_functions[] = {
    "FT_Read",
    "FT_Write",
    "USBIntLED_Init",
};

static const size_t target_modules_len = _countof(target_modules);
static const size_t target_functions_len = _countof(target_functions);

void elisabeth_hook_init()
{
    dll_hook_insert_hooks(NULL);
}

static void dll_hook_insert_hooks(HMODULE target)
{
    hook_table_apply(
            target,
            "kernel32.dll",
            elisabeth_hooks,
            _countof(elisabeth_hooks));
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
            path_hook_insert_hooks(result);
        }
    }

    return result;
}

FARPROC WINAPI my_GetProcAddress(HMODULE hModule, const char *name)
{
    uintptr_t ordinal;

    FARPROC result = next_GetProcAddress(hModule, name);


    for (size_t i = 0; i < target_functions_len; i++) {
        ordinal = (uintptr_t) name;

        if (ordinal > 0xFFFF) {
            /* Import by name */
            if (strcmp(target_functions[i], name) == 0)
                dprintf("Elisabeth: GetProcAddress %s is %p\n", name, result);
        }
    }

    return result;
}
