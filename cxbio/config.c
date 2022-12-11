#include <windows.h>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "cxbio/config.h"

void cxb_io_config_load(
        struct cxb_io_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->test = GetPrivateProfileIntW(L"revio", L"test", '1', filename);
    cfg->service = GetPrivateProfileIntW(L"revio", L"service", '2', filename);
    cfg->coin = GetPrivateProfileIntW(L"revio", L"coin", '3', filename);
    cfg->cancel = GetPrivateProfileIntW(L"revio", L"cancel", '4', filename);
    cfg->up = GetPrivateProfileIntW(L"revio", L"up", VK_UP, filename);
    cfg->down = GetPrivateProfileIntW(L"revio", L"down", VK_DOWN, filename);
}
