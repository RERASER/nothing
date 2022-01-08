#include <windows.h>

#include <limits.h>
#include <stdint.h>

#include "mercuryio/mercuryio.h"
#include "mercuryio/config.h"

static uint8_t mercury_opbtn;
static uint8_t mercury_gamebtn;
static struct mercury_io_config mercury_io_cfg;

uint16_t mercury_io_get_api_version(void)
{
    return 0x0100;
}

HRESULT mercury_io_init(void)
{
    mercury_io_config_load(&mercury_io_cfg, L".\\segatools.ini");

    return S_OK;
}

HRESULT mercury_io_poll(void)
{
    mercury_opbtn = 0;
    mercury_gamebtn = 0;

    if (GetAsyncKeyState(mercury_io_cfg.vk_test)) {
        mercury_opbtn |= MERCURY_IO_OPBTN_TEST;
    }

    if (GetAsyncKeyState(mercury_io_cfg.vk_service)) {
        mercury_opbtn |= MERCURY_IO_OPBTN_SERVICE;
    }

    if (GetAsyncKeyState(mercury_io_cfg.vk_vol_up)) {
        mercury_gamebtn |= MERCURY_IO_GAMEBTN_VOL_UP;
    }

    if (GetAsyncKeyState(mercury_io_cfg.vk_vol_down)) {
        mercury_gamebtn |= MERCURY_IO_GAMEBTN_VOL_DOWN;
    }

    return S_OK;
}

void mercury_io_get_opbtns(uint8_t *opbtn)
{
    if (opbtn != NULL) {
        *opbtn = mercury_opbtn;
    }
}

void mercury_io_get_gamebtns(uint8_t *gamebtn)
{
    if (gamebtn != NULL) {
        *gamebtn = mercury_gamebtn;
    }
}

HRESULT mercury_io_touch_init(void)
{
    return S_OK;
}

HRESULT mercury_io_touch_start(void)
{
    return S_OK;
}
