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
static FARPROC WINAPI my_GetProcAddress(HMODULE hModule, const char *name);
static FARPROC (WINAPI *next_GetProcAddress)(HMODULE hModule, const char *name);
static int my_USBIntLED_Init();

static const struct hook_symbol win32_hooks[] = {
    {
        .name = "GetProcAddress",
        .patch = my_GetProcAddress,
        .link = (void **) &next_GetProcAddress
    }
};

HRESULT elisabeth_hook_init()
{
    dll_hook_insert_hooks(NULL);
    return S_OK;
}

static void dll_hook_insert_hooks(HMODULE target)
{
    hook_table_apply(
            target,
            "kernel32.dll",
            win32_hooks,
            _countof(win32_hooks));
}

FARPROC WINAPI my_GetProcAddress(HMODULE hModule, const char *name)
{
    uintptr_t ordinal = (uintptr_t) name;

    FARPROC result = next_GetProcAddress(hModule, name);

    if (ordinal > 0xFFFF) {
        /* Import by name */
        if (strcmp(name, "USBIntLED_Init") == 0) {
            result = (FARPROC) my_USBIntLED_Init;
        }
    }

    return result;
}

/* Intercept the call to initialize the LED board. */
static int my_USBIntLED_Init()
{
    dprintf("Elisabeth: my_USBIntLED_Init hit!\n");
    return 1;
}
