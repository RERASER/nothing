#pragma once

#include <windows.h>

#include <stddef.h>

HRESULT setupapi_add_phantom_dev(const GUID *iface_class, const wchar_t *path);
void setupapi_hook_insert_hooks(HMODULE target);
