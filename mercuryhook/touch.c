#include <windows.h>

#include <assert.h>
#include <process.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "board/slider-cmd.h"
#include "board/slider-frame.h"

#include "mercuryhook/mercury-dll.h"
#include "mercuryhook/touch.h"

#include "hook/iobuf.h"
#include "hook/iohook.h"

#include "hooklib/uart.h"

#include "util/dprintf.h"
#include "util/dump.h"

static HRESULT touch0_handle_irp(struct irp *irp);
static HRESULT touch0_handle_irp_locked(struct irp *irp);

static HRESULT touch_req_dispatch(const union slider_req_any *req);

static void touch_res_auto_scan(const uint8_t *state);

static CRITICAL_SECTION touch0_lock;
static struct uart touch0_uart;
static uint8_t touch0_written_bytes[520];
static uint8_t touch0_readable_bytes[520];

static CRITICAL_SECTION touch1_lock;
static struct uart touch1_uart;
static uint8_t touch1_written_bytes[520];
static uint8_t touch1_readable_bytes[520];

HRESULT touch_hook_init(const struct touch_config *cfg)
{    
    assert(cfg != NULL);
    assert(mercury_dll.touch_init != NULL);    

    // not sure why this always returns false...
    /*if (!cfg->enable) {
        return S_FALSE;
    }*/

    InitializeCriticalSection(&touch0_lock);
    InitializeCriticalSection(&touch1_lock);
    dprintf("Wacca touch: init\n");

    uart_init(&touch0_uart, 3);
    touch0_uart.written.bytes = touch0_written_bytes;
    touch0_uart.written.nbytes = sizeof(touch0_written_bytes);
    touch0_uart.readable.bytes = touch0_readable_bytes;
    touch0_uart.readable.nbytes = sizeof(touch0_readable_bytes);

    uart_init(&touch1_uart, 4);
    touch1_uart.written.bytes = touch1_written_bytes;
    touch1_uart.written.nbytes = sizeof(touch1_written_bytes);
    touch1_uart.readable.bytes = touch1_readable_bytes;
    touch1_uart.readable.nbytes = sizeof(touch1_readable_bytes);

    return iohook_push_handler(touch0_handle_irp);
}

static HRESULT touch0_handle_irp(struct irp *irp)
{
    HRESULT hr;

    assert(irp != NULL);

    if (!uart_match_irp(&touch0_uart, irp)) {
        return iohook_invoke_next(irp);
    }

    EnterCriticalSection(&touch0_lock);
    hr = touch0_handle_irp_locked(irp);
    LeaveCriticalSection(&touch0_lock);

    return hr;
}

static HRESULT touch0_handle_irp_locked(struct irp *irp)
{
    union slider_req_any req;
    struct iobuf req_iobuf;
    HRESULT hr;

    if (irp->op == IRP_OP_OPEN) {
        dprintf("Wacca touch: Starting backend\n");
        hr = mercury_dll.touch_init();

        if (FAILED(hr)) {
            dprintf("Wacca touch: Backend error: %x\n", (int) hr);

            return hr;
        }
    }

    hr = uart_handle_irp(&touch0_uart, irp);

    if (FAILED(hr) || irp->op != IRP_OP_WRITE) {
        return hr;
    }

    for (;;) {
#if 0
        dprintf("TX Buffer:\n");
        dump_iobuf(&touch0_uart.written);
#endif

        req_iobuf.bytes = req.bytes;
        req_iobuf.nbytes = sizeof(req.bytes);
        req_iobuf.pos = 0;

        hr = slider_frame_decode(&req_iobuf, &touch0_uart.written);

        if (hr != S_OK) {
            if (FAILED(hr)) {
                dprintf("Wacca touch: Deframe error: %x\n", (int) hr);
            }

            return hr;
        }

#if 0
        dprintf("Deframe Buffer:\n");
        dump_iobuf(&req_iobuf);
#endif

        hr = touch_req_dispatch(&req);

        if (FAILED(hr)) {
            dprintf("Wacca touch: Processing error: %x\n", (int) hr);
        }
    }
}

static HRESULT touch_req_dispatch(const union slider_req_any *req)
{
    switch (req->hdr.cmd) {
    default:
        dprintf("Unhandled command %02x\n", req->hdr.cmd);

        return S_OK;
    }
}

static void slider_res_auto_scan(const uint8_t *state)
{
    struct slider_resp_auto_scan resp;

    resp.hdr.sync = SLIDER_FRAME_SYNC;
    resp.hdr.cmd = SLIDER_CMD_AUTO_SCAN;
    resp.hdr.nbytes = sizeof(resp.pressure);
    memcpy(resp.pressure, state, sizeof(resp.pressure));

    EnterCriticalSection(&touch0_lock);
    slider_frame_encode(&touch0_uart.readable, &resp, sizeof(resp));
    LeaveCriticalSection(&touch0_lock);
}
