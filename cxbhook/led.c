#include <windows.h>
#include <stdbool.h>
#include <stdint.h>

#include "cxbhook/led.h"
#include "cxbhook/cxb-dll.h"

#include "hooklib/procaddr.h"

#include "hook/table.h"

#include "util/dprintf.h"


static int my_cCommLamp_Open(char *port);
static void my_cCommLamp_Close();
static int my_cCommLamp_Setup(int led_id);
static int my_cCommLamp_SetColor(int led_id, int color);
static int my_cCommLamp_Update();
static int my_cCommLamp_UpdateDelta(float delta);


static struct hook_symbol lamp_syms[] = {
    {
        .name = "cCommLamp_Open",
        .patch = my_cCommLamp_Open
    },
    {
        .name = "cCommLamp_Close",
        .patch = my_cCommLamp_Close
    },
    {
        .name = "cCommLamp_Setup",
        .patch = my_cCommLamp_Setup,
    },
    {
        .name = "cCommLamp_SetColor",
        .patch = my_cCommLamp_SetColor
    },
    {
        .name = "cCommLamp_Update",
        .patch = my_cCommLamp_Update
    },
    {
        .name = "cCommLamp_UpdateDelta",
        .patch = my_cCommLamp_UpdateDelta
    },
};

HRESULT led_hook_init(struct led_config *cfg)
{
    dprintf("LED: Init\n");
    return proc_addr_table_push("CommLamp.dll", lamp_syms, _countof(lamp_syms));
}

static int my_cCommLamp_Open(char *port)
{
    HRESULT hr = cxb_dll.led_init();
    dprintf("LED: Open %s (DLL init result %lx)\n", port, hr);

    if (FAILED(hr)) {
        return 0;
    }

    return 1;
}

static void my_cCommLamp_Close()
{
    dprintf("LED: Close\n");
}

static int my_cCommLamp_Setup(int led_id)
{
    dprintf("LED: Setup %d\n", led_id);
    return 0;
}

static int my_cCommLamp_SetColor(int led_id, int color)
{
    cxb_dll.led_update(led_id, color);
    return 1;
}

static int my_cCommLamp_Update()
{
    return 0;
}

static int my_cCommLamp_UpdateDelta(float delta)
{
    return 0;
}