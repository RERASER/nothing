#include <windows.h>

#include <process.h>
#include <stdbool.h>
#include <stdint.h>

#include "cxbio/cxbio.h"
#include "cxbio/config.h"

#include "util/dprintf.h"

static bool cxb_io_coin;
static int cxb_io_coins;
static struct cxb_io_config cxb_io_cfg;

uint16_t cxb_io_get_api_version(void)
{
    return 0x0100;
}

HRESULT cxb_io_revio_init(void)
{
    dprintf("CXB IO: REVIO init\n");
    cxb_io_config_load(&cxb_io_cfg, L".\\segatools.ini");

    return S_OK;
}

void cxb_io_revio_poll(uint16_t *opbtn)
{
    if (GetAsyncKeyState(cxb_io_cfg.test)) {
        *opbtn |= 0x01; /* Test */
    }

    if (GetAsyncKeyState(cxb_io_cfg.service)) {
        *opbtn |= 0x02; /* Service */
    }

    if (GetAsyncKeyState(cxb_io_cfg.cancel)) {
        *opbtn |= 0x04; /* Cancel */
    }

    if (GetAsyncKeyState(cxb_io_cfg.up)) {
        *opbtn |= 0x08; /* Up */
    }

    if (GetAsyncKeyState(cxb_io_cfg.down)) {
        *opbtn |= 0x10; /* Down */
    }
}

void cxb_io_revio_get_coins(long *coins)
{
    if (GetAsyncKeyState(cxb_io_cfg.coin)) {
        if (!cxb_io_coin) {
            cxb_io_coin = true;
            cxb_io_coins++;
        }
    } else {
        cxb_io_coin = false;
    }

    *coins = cxb_io_coins;
}

void cxb_io_revio_set_coins(int coins)
{
    cxb_io_coins = coins;
}

HRESULT cxb_io_led_init(void)
{
    dprintf("CXB IO: LED init\n");
    return S_OK;
}

void cxb_io_led_update(int id, int color)
{}