#include <windows.h>

#include <limits.h>
#include <stdint.h>
#include <process.h>

#include "mercuryio/mercuryio.h"
#include "mercuryio/config.h"

static unsigned int __stdcall mercury_io_touch_thread_proc(void *ctx);

static uint8_t mercury_opbtn;
static uint8_t mercury_gamebtn;
static struct mercury_io_config mercury_io_cfg;
static bool mercury_io_touch_stop_flag;
static HANDLE mercury_io_touch_thread;

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

void mercury_io_touch_start(mercury_io_touch_callback_t callback)
{
    if (mercury_io_touch_thread != NULL) {
        return;
    }

    mercury_io_touch_thread = (HANDLE) _beginthreadex(
        NULL,
        0,
        mercury_io_touch_thread_proc,
        callback,
        0,
        NULL
    );
}

static unsigned int __stdcall mercury_io_touch_thread_proc(void *ctx)
{
    mercury_io_touch_callback_t callback;
    uint8_t pressure[240];
    size_t i;

    callback = ctx;

    while (!mercury_io_touch_stop_flag) {
        for (i = 0 ; i < _countof(pressure) ; i++) {
            if (GetAsyncKeyState(mercury_io_cfg.vk_cell[i]) & 0x8000) {
                pressure[i] = 128;
            } else {
                pressure[i] = 0;
            }
        }

        callback(pressure);
        Sleep(1);
    }

    return 0;
}
