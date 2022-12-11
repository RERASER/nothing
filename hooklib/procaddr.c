#include <windows.h>
#include <stdbool.h>
#include <stdint.h>
#include <libgen.h>

#include "hooklib/procaddr.h"

#include "hook/table.h"

#include "util/dprintf.h"

static struct proc_addr_table *proc_addr_hook_list;
static size_t proc_addr_hook_count;
static CRITICAL_SECTION proc_addr_hook_lock;
static bool proc_addr_hook_initted;

static FARPROC WINAPI my_GetProcAddress(HMODULE hModule, const char *name);
static FARPROC (WINAPI *next_GetProcAddress)(HMODULE hModule, const char *name);
static void proc_addr_hook_init(void);

static const struct hook_symbol win32_hooks[] = {
    {
        .name = "GetProcAddress",
        .patch = my_GetProcAddress,
        .link = (void **) &next_GetProcAddress
    }
};

HRESULT proc_addr_table_push(
    const char *target,
    struct hook_symbol *syms,
    size_t nsyms
)
{
    HRESULT hr;
    struct proc_addr_table *new_item;
    struct proc_addr_table *new_mem;

    proc_addr_hook_init();

    EnterCriticalSection(&proc_addr_hook_lock);

    new_mem = realloc(
            proc_addr_hook_list,
            (proc_addr_hook_count + 1) * sizeof(struct proc_addr_table));
    
    if (new_mem == NULL) {
        hr = E_OUTOFMEMORY;
        
        LeaveCriticalSection(&proc_addr_hook_lock);
        return hr;
    }

    new_item = &new_mem[proc_addr_hook_count];
    new_item->name = target;    
    new_item->nsyms = nsyms;
    new_item->syms = syms;

    proc_addr_hook_list = new_mem;
    proc_addr_hook_count++;
    hr = S_OK;

    LeaveCriticalSection(&proc_addr_hook_lock);

    return hr;
}

static void proc_addr_hook_init(void)
{
    if (proc_addr_hook_initted) {
        return;
    }

    dprintf("ProcAddr: Hook init\n");
    proc_addr_hook_initted = true;

    InitializeCriticalSection(&proc_addr_hook_lock);

    hook_table_apply(
            NULL,
            "kernel32.dll",
            win32_hooks,
            _countof(win32_hooks));
}

FARPROC WINAPI my_GetProcAddress(HMODULE hModule, const char *name)
{
    uintptr_t ordinal = (uintptr_t) name;
    char mod_path[PATH_MAX];
    char *mod_name;
    const struct hook_symbol *sym;
    FARPROC result = next_GetProcAddress(hModule, name);
    
    GetModuleFileNameA(hModule, mod_path, PATH_MAX);
    mod_name = basename(mod_path);

    for (int i = 0; i < proc_addr_hook_count; i++) {

        if (strcmp(proc_addr_hook_list[i].name, mod_name) == 0) {
            
            for (int j = 0; j < proc_addr_hook_list[i].nsyms; j++) {
                sym = &proc_addr_hook_list[i].syms[j];
                
                if (ordinal > 0xFFFF) {

                    if (strcmp(sym->name, name) == 0) {

                        dprintf("ProcAddr: Hooking %s from %s\n", name, mod_name);
                        result = (FARPROC) sym->patch;
                    }
                }

                else {
                    if (sym->ordinal == ordinal) {

                        dprintf("ProcAddr: Hooking Ord %p from %s\n", (void *)ordinal, mod_name);
                        result = (FARPROC) sym->patch;
                    }
                }
            }
        }
    }

    return result;
}