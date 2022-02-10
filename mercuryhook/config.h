#pragma once

#include <stddef.h>

#include "board/config.h"

#include "hooklib/dvd.h"
#include "gfxhook/gfx.h"

#include "mercuryhook/mercury-dll.h"
#include "mercuryhook/touch.h"

#include "platform/config.h"

struct mercury_hook_config {
    struct platform_config platform;
    struct aime_config aime;
    struct dvd_config dvd;
    struct io4_config io4;
    struct gfx_config gfx;
    struct mercury_dll_config dll;
    struct touch_config touch;
};

void mercury_dll_config_load(
        struct mercury_dll_config *cfg,
        const wchar_t *filename);

void mercury_hook_config_load(
        struct mercury_hook_config *cfg,
        const wchar_t *filename);
