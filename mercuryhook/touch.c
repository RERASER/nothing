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
#include "hooklib/fdshark.h"

#include "util/dprintf.h"
#include "util/dump.h"

static HRESULT touch0_handle_irp(struct irp *irp);
static HRESULT touch0_handle_irp_locked(struct irp *irp);
static HRESULT touch1_handle_irp(struct irp *irp);
static HRESULT touch1_handle_irp_locked(struct irp *irp);

static HRESULT touch_req_dispatch(const struct touch_req *req);

static HRESULT touch_frame_decode(struct touch_req *dest, const struct iobuf *iobuf, int side);

static HRESULT touch_handle_get_rev_date(const struct touch_req *req);

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

    iohook_push_handler(touch0_handle_irp);
    iohook_push_handler(touch1_handle_irp);
    return S_OK;
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
    struct touch_req req;
    HRESULT hr;

    if (irp->op == IRP_OP_OPEN) {
        dprintf("Wacca touch0: Starting backend\n");
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
#if 1
        dprintf("TX0 Buffer:\n");
        dump_iobuf(&touch0_uart.written);
#endif
        hr = touch_frame_decode(&req, &touch0_uart.written, 0);

        if (hr != S_OK) {
            if (FAILED(hr)) {
                dprintf("Wacca touch: Deframe error: %x\n", (int) hr);
            }

            return hr;
        }

        hr = touch_req_dispatch(&req);

        if (FAILED(hr)) {
            dprintf("Wacca touch: Processing error: %x\n", (int) hr);
        }
    }
}

static HRESULT touch1_handle_irp(struct irp *irp)
{
    HRESULT hr;

    assert(irp != NULL);

    if (!uart_match_irp(&touch1_uart, irp)) {
        return iohook_invoke_next(irp);
    }

    EnterCriticalSection(&touch1_lock);
    hr = touch1_handle_irp_locked(irp);
    LeaveCriticalSection(&touch1_lock);

    return hr;
}

static HRESULT touch1_handle_irp_locked(struct irp *irp)
{
    struct touch_req req;
    HRESULT hr;

    if (irp->op == IRP_OP_OPEN) {
        dprintf("Wacca touch1: Starting backend\n");
        hr = mercury_dll.touch_init();

        if (FAILED(hr)) {
            dprintf("Wacca touch: Backend error: %x\n", (int) hr);

            return hr;
        }
    }

    hr = uart_handle_irp(&touch1_uart, irp);

    if (FAILED(hr) || irp->op != IRP_OP_WRITE) {
        return hr;
    }

    for (;;) {
#if 1
        dprintf("TX1 Buffer:\n");
        dump_iobuf(&touch1_uart.written);
#endif

        hr = touch_frame_decode(&req, &touch0_uart.written, 1);

        if (hr != S_OK) {
            if (FAILED(hr)) {
                dprintf("Wacca touch: Deframe error: %x\n", (int) hr);
            }

            return hr;
        }

        hr = touch_req_dispatch(&req);

        if (FAILED(hr)) {
            dprintf("Wacca touch: Processing error: %x\n", (int) hr);
        }
    }
}

static HRESULT touch_req_dispatch(const struct touch_req *req)
{
    switch (req->cmd) {
    case CMD_GET_REV_DATE:
        return touch_handle_get_rev_date(req);
    default:
        dprintf("Wacca touch: Unhandled command %02x\n", req->cmd);

        return S_OK;
    }
}

static HRESULT touch_handle_get_rev_date(const struct touch_req *req)
{
    dprintf("Wacca Touch%d: Get board rev date\n", req->side);
    if (req->side) {
        touch0_uart.readable.bytes[0] = 0xa0;
        touch0_uart.readable.bytes[1] = 0x31;
        touch0_uart.readable.bytes[2] = 0x39;
        touch0_uart.readable.bytes[3] = 0x30;
        touch0_uart.readable.bytes[4] = 0x35;
        touch0_uart.readable.bytes[5] = 0x32;
        touch0_uart.readable.bytes[6] = 0x33;
        touch0_uart.readable.bytes[7] = 0x2c;
    }
    else {
        touch1_uart.readable.bytes[0] = 0xa0;
        touch1_uart.readable.bytes[1] = 0x31;
        touch1_uart.readable.bytes[2] = 0x39;
        touch1_uart.readable.bytes[3] = 0x30;
        touch1_uart.readable.bytes[4] = 0x35;
        touch1_uart.readable.bytes[5] = 0x32;
        touch1_uart.readable.bytes[6] = 0x33;
        touch1_uart.readable.bytes[7] = 0x2c;
    }
    return S_OK;
}

static HRESULT touch_frame_decode(struct touch_req *dest, const struct iobuf *iobuf, int side)
{
    dest->side = side;
    dest->cmd = iobuf->bytes[0];
    dest->data_length = _countof(iobuf->bytes) - 1;

    if (dest->data_length > 0) {
        for (int i = 1; i < dest->data_length; i++) {
            dest->data[i-1] = iobuf->bytes[i];
        }
    }

    return S_OK;
}
