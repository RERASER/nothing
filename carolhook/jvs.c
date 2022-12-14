#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "amex/jvs.h"

#include "board/io3.h"

#include "carolhook/carol-dll.h"

#include "jvs/jvs-bus.h"

#include "util/dprintf.h"

static void carol_jvs_read_switches(void *ctx, struct io3_switch_state *out);
static void carol_jvs_read_coin_counter(
        void *ctx,
        uint8_t slot_no,
        uint16_t *out);

static const struct io3_ops carol_jvs_io3_ops = {
    .read_switches      = carol_jvs_read_switches,
    .read_coin_counter  = carol_jvs_read_coin_counter,
};

static struct io3 carol_jvs_io3;

HRESULT carol_jvs_init(struct jvs_node **out)
{
    HRESULT hr;

    assert(out != NULL);
    assert(carol_dll.jvs_init != NULL);

    dprintf("JVS I/O: Starting carol backend DLL\n");
    hr = carol_dll.jvs_init();

    if (FAILED(hr)) {
        dprintf("JVS I/O: Backend error, I/O disconnected: %x\n", (int) hr);

        return hr;
    }

    io3_init(&carol_jvs_io3, NULL, &carol_jvs_io3_ops, NULL);
    *out = io3_to_jvs_node(&carol_jvs_io3);

    return S_OK;
}

static void carol_jvs_read_switches(void *ctx, struct io3_switch_state *out)
{
    uint8_t opbtn;
    uint8_t gamebtn;

    assert(out != NULL);
    assert(carol_dll.jvs_poll != NULL);

    opbtn = 0;
    gamebtn = 0;

    carol_dll.jvs_poll(&opbtn, &gamebtn);

    if (gamebtn & 0x01) {
        out->p1 |= 1 << 6;
    }

    if (gamebtn & 0x02) {
        out->p1 |= 1 << 7;
    }

    if (gamebtn & 0x04) {
        out->p1 |= 1 << 8;
    }

    if (gamebtn & 0x08) {
        out->p1 |= 1 << 9;
    }

    if (gamebtn & 0x10) {
        out->p1 |= 1 << 15;
    }

    if (opbtn & 0x01) {
        out->system = 0x80;
    } else {
        out->system = 0;
    }

    if (opbtn & 0x02) {
        out->p1 |= 1 << 14;
    }
}

static void carol_jvs_read_coin_counter(
        void *ctx,
        uint8_t slot_no,
        uint16_t *out)
{
    assert(carol_dll.jvs_read_coin_counter != NULL);

    if (slot_no > 0) {
        return;
    }

    carol_dll.jvs_read_coin_counter(out);
}
