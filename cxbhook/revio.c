#include <windows.h>
#include <stdbool.h>
#include <stdint.h>
#include <winuser.h>

#include "cxbhook/revio.h"
#include "cxbhook/cxb-dll.h"

#include "hooklib/procaddr.h"

#include "hook/table.h"

#include "util/dprintf.h"

static int my_cCommIo_Open(char *port);
static int my_cCommIo_Close();
static long my_cCommIo_GetCoin();
static int my_cCommIo_SetCoin(int coin_ct);
static int my_cCommIo_GetStatus();
static int my_cCommIo_GetSwitch();
static int my_cCommIo_GetTrigger();
static int my_cCommIo_GetRelease();
static long my_cCommIo_GetVolume();
static int my_cCommIo_SetAmpVolume(int amp_id, long new_volume);
static int my_cCommIo_GetAmpVolume(int amp_id);
static int my_cCommIo_SetAmpMute(int amp_id, int a2);

int amp_volume[] = {20, 20, 20};
int last_triggers = 0;
int last_is_mouse_down = false;

static struct hook_symbol revio_syms[] = {
    {
        .name = "cCommIo_Open",
        .patch = my_cCommIo_Open
    },
    {
        .name = "cCommIo_Close",
        .patch = my_cCommIo_Close
    },    
    {
        .name = "cCommIo_GetStatus",
        .patch = my_cCommIo_GetStatus
    },    
    {
        .name = "cCommIo_GetCoin",
        .patch = my_cCommIo_GetCoin
    },
    {
        .name = "cCommIo_SetCoin",
        .patch = my_cCommIo_SetCoin
    },
    {
        .name = "cCommIo_GetSwitch",
        .patch = my_cCommIo_GetSwitch
    },
    {
        .name = "cCommIo_GetTrigger",
        .patch = my_cCommIo_GetTrigger
    },
    {
        .name = "cCommIo_GetRelease",
        .patch = my_cCommIo_GetRelease
    },
    {
        .name = "cCommIo_GetVolume",
        .patch = my_cCommIo_GetVolume
    },
    {
        .name = "cCommIo_SetAmpVolume",
        .patch = my_cCommIo_SetAmpVolume
    },
    {
        .name = "cCommIo_GetAmpVolume",
        .patch = my_cCommIo_GetAmpVolume
    },
    {
        .name = "cCommIo_SetAmpMute",
        .patch = my_cCommIo_SetAmpMute
    },
};

HRESULT revio_hook_init(struct revio_config *cfg)
{
    dprintf("Revio: Init\n");
    return proc_addr_table_push("CommIo.dll", revio_syms, _countof(revio_syms));
}

static int my_cCommIo_Open(char *port)
{
    dprintf("Revio: Open port %s\n", port);
    cxb_dll.revio_init();
    return 1;
}

static int my_cCommIo_Close()
{
    dprintf("Revio: Close\n");
    return 0;
}

static int my_cCommIo_GetStatus()
{
    return 1;
}

static long my_cCommIo_GetCoin()
{
    long coins;
    cxb_dll.revio_get_coins(&coins);

    return coins;
}

static int my_cCommIo_SetCoin(int coin_ct)
{
    // does some weird shit, not sure
    //dprintf("Revio: Set coin %d\n", coin_ct);
    cxb_dll.revio_set_coins(coin_ct);
    return 1;
}

static int my_cCommIo_GetSwitch()
{
    return 0;
}

static int my_cCommIo_GetTrigger()
{
    uint16_t btns = 0;
    int out = 0;

    cxb_dll.revio_poll(&btns);

    if (btns & 0x01) {
        out |= 1 << 4; // test
    }

    if (btns & 0x02) {
        out |= 1 << 5; // service?
    }

    if (btns & 0x04) {
        out |= 1 << 1; // up
    }

    if (btns & 0x08) {
        out |= 1 << 3; // down
    }

    if (btns & 0x0F) {
        out |= 1 << 2; // cancel
    }
    
    out &= ~last_triggers;

    dprintf("Revio: GetTrigger %X\n", out);
    last_triggers = out;
    return out;
}

static int my_cCommIo_GetRelease()
{
    uint16_t btns = 0;
    int out = last_triggers;

    cxb_dll.revio_poll(&btns);

    if (btns & 0x01) {
        out |= 1 << 4; // test
    }

    if (btns & 0x02) {
        out |= 1 << 5; // service?
    }

    if (btns & 0x04) {
        out |= 1 << 1; // up
    }

    if (btns & 0x08) {
        out |= 1 << 3; // down
    }

    if (btns & 0x0F) {
        out |= 1 << 2; // cancel
    }

    out &= ~btns;
    
    dprintf("Revio: GetRelease %X\n", out);
    last_triggers = btns;
    return out;
}

static long my_cCommIo_GetVolume()
{
    return 0;
}

static int my_cCommIo_SetAmpVolume(int amp_id, long new_volume)
{
    dprintf("Revio: SetAmpVolume id %d -> vol %ld\n", amp_id, new_volume);
    if (amp_id > _countof(amp_volume)) {
        return 0;
    }

    amp_volume[amp_id] = new_volume;
    return 0;
}

static int my_cCommIo_GetAmpVolume(int amp_id)
{
    dprintf("Revio: GetAmpVolume id %d\n", amp_id);
     if (amp_id > _countof(amp_volume)) {
        return 0;
    }

    return amp_volume[amp_id];
}

static int my_cCommIo_SetAmpMute(int amp_id, int a2)
{
    dprintf("Revio: GetAmpVolume id %d unknown %d\n", amp_id, a2);
    return 0;
}
