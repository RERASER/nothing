#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "amex/amex.h"

#include "board/sg-reader.h"

#include "cxbhook/cxb-dll.h"
#include "cxbhook/revio.h"
#include "cxbhook/led.h"
#include "cxbhook/network.h"

#include "gfxhook/gfx.h"

#include "platform/platform.h"

struct cxb_hook_config {
    struct platform_config platform;
    struct amex_config amex;
    struct aime_config aime;
    struct gfx_config gfx;
    struct cxb_dll_config dll;
    struct revio_config revio;
    struct network_config network;
    struct led_config led;
};

void cxb_dll_config_load(
        struct cxb_dll_config *cfg,
        const wchar_t *filename);

void revio_config_load(struct revio_config *cfg, const wchar_t *filename);
void network_config_load(struct network_config *cfg, const wchar_t *filename);
void led_config_load(struct led_config *cfg, const wchar_t *filename);

void cxb_hook_config_load(
        struct cxb_hook_config *cfg,
        const wchar_t *filename);
