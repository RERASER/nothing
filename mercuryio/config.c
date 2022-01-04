#include <windows.h>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "mercuryio/config.h"

/*
Wacca Default key binding
*/

void mercury_io_config_load(
        struct mercury_io_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->vk_test = GetPrivateProfileIntW(L"io4", L"test", '1', filename);
    cfg->vk_service = GetPrivateProfileIntW(L"io4", L"service", '2', filename);
}
