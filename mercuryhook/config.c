#include <assert.h>
#include <stddef.h>

#include "board/config.h"

#include "hooklib/config.h"
#include "hooklib/dvd.h"

#include "mercuryhook/config.h"

#include "platform/config.h"

void mercury_dll_config_load(
        struct mercury_dll_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    GetPrivateProfileStringW(
            L"mercuryio",
            L"path",
            L"",
            cfg->path,
            _countof(cfg->path),
            filename);
}

void touch_config_load(
        struct touch_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(
            L"touch",
            L"enable",
            1,
            filename);
}

void mercury_hook_config_load(
        struct mercury_hook_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    platform_config_load(&cfg->platform, filename);
    aime_config_load(&cfg->aime, filename);
    dvd_config_load(&cfg->dvd, filename);
    io4_config_load(&cfg->io4, filename);
    mercury_dll_config_load(&cfg->dll, filename);
    touch_config_load(&cfg->touch, filename);
}
