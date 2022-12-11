#pragma once
#include <windows.h>
#include <stdbool.h>
#include <stdint.h>

#include "hook/table.h"

struct proc_addr_table {
    const char *name;
    size_t nsyms;    
    struct hook_symbol *syms;
};

HRESULT proc_addr_table_push(
    const char *target,
    struct hook_symbol *syms,
    size_t nsyms
);