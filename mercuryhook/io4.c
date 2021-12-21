#include <windows.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "board/io4.h"

#include "mercuryhook/mercury-dll.h"

#include "util/dprintf.h"

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

    mercury_dll.get_opbtns(&opbtn);

    if (opbtn & MAI2_IO_OPBTN_TEST) {
        state->buttons[0] |= IO4_BUTTON_TEST;
    }

    if (opbtn & MAI2_IO_OPBTN_SERVICE) {
        state->buttons[0] |= IO4_BUTTON_SERVICE;
    }

    return S_OK;
}
