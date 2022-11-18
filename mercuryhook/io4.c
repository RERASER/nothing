#include <windows.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "board/io4.h"

#include "mercuryhook/mercury-dll.h"

#include "util/dprintf.h"

bool mercury_io_coin = false;
uint16_t mercury_io_coins = 0;

static HRESULT mercury_io4_poll(void *ctx, struct io4_state *state);

static const struct io4_ops mercury_io4_ops = {
    .poll = mercury_io4_poll,
};

HRESULT mercury_io4_hook_init(const struct io4_config *cfg)
{
    HRESULT hr;

    assert(mercury_dll.init != NULL);

    hr = io4_hook_init(cfg, &mercury_io4_ops, NULL);

    if (FAILED(hr)) {
        return hr;
    }

    return mercury_dll.init();
}

static HRESULT mercury_io4_poll(void *ctx, struct io4_state *state)
{
    uint8_t opbtn;
    uint8_t gamebtn;
    HRESULT hr;

    assert(mercury_dll.poll != NULL);
    assert(mercury_dll.get_opbtns != NULL);
    assert(mercury_dll.get_gamebtns != NULL);

    memset(state, 0, sizeof(*state));

    hr = mercury_dll.poll();

    if (FAILED(hr)) {
        return hr;
    }

    opbtn = 0;
    gamebtn = 0;

    mercury_dll.get_opbtns(&opbtn);
    mercury_dll.get_gamebtns(&gamebtn);

    if (opbtn & MERCURY_IO_OPBTN_TEST) {
        state->buttons[0] |= IO4_BUTTON_TEST;
    }

    if (opbtn & MERCURY_IO_OPBTN_SERVICE) {
        state->buttons[0] |= IO4_BUTTON_SERVICE;
    }

    if (opbtn & MERCURY_IO_OPBTN_COIN) {
        if (!mercury_io_coin) {
            mercury_io_coin = true;
            mercury_io_coins++;
        }
    }
    else {
        mercury_io_coin = false;
    }

    state->chutes[0] = 128 + 256 * mercury_io_coins;

    if (gamebtn & MERCURY_IO_GAMEBTN_VOL_UP) {
        state->buttons[0] |= 1 << 1;
    }

    if (gamebtn & MERCURY_IO_GAMEBTN_VOL_DOWN) {
        state->buttons[0] |= 1 << 0;
    }

    return S_OK;
}
