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

static HRESULT touch_handle_irp(struct irp *irp);
static HRESULT touch0_handle_irp_locked(struct irp *irp);
static HRESULT touch1_handle_irp_locked(struct irp *irp);

static HRESULT touch_req_dispatch(const struct touch_req *req);

static HRESULT touch_frame_decode(struct touch_req *dest, struct iobuf *iobuf, int side);
static HRESULT touch_frame_encode(struct iobuf *dest, const void *ptr, size_t nbytes);
static uint8_t calc_checksum(const void *ptr, size_t nbytes);

static HRESULT touch_handle_get_rev_date(const struct touch_req *req);
static HRESULT touch_handle_startup(const struct touch_req *req);
static HRESULT touch_handle_get_rev_date_detail(const struct touch_req *req);
static HRESULT touch_handle_mystery1(const struct touch_req *req);
static HRESULT touch_handle_mystery2(const struct touch_req *req);
static HRESULT touch_handle_start_auto_scan(const struct touch_req *req);

uint8_t input_frame_count = 0x7b;

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

    return iohook_push_handler(touch_handle_irp);
}

static HRESULT touch_handle_irp(struct irp *irp)
{
    HRESULT hr;

    assert(irp != NULL);

    if (uart_match_irp(&touch0_uart, irp)) {
        EnterCriticalSection(&touch0_lock);
        hr = touch0_handle_irp_locked(irp);
        LeaveCriticalSection(&touch0_lock);
    }
    else if (uart_match_irp(&touch1_uart, irp)) {
        EnterCriticalSection(&touch1_lock);
        hr = touch1_handle_irp_locked(irp);
        LeaveCriticalSection(&touch1_lock);
    }
    else {
        return iohook_invoke_next(irp);
    }

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
#if 0
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

        return hr;
    }
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
#if 0
        dprintf("TX1 Buffer:\n");
        dump_iobuf(&touch1_uart.written);
#endif

        hr = touch_frame_decode(&req, &touch1_uart.written, 1);

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

        return hr;
    }
}

static HRESULT touch_req_dispatch(const struct touch_req *req)
{
    switch (req->cmd) {
    case CMD_GET_REV_DATE:
        return touch_handle_get_rev_date(req);
    case CMD_STARTUP:
        return touch_handle_startup(req);
    case CMD_GET_REV_DATE_DETAIL:
        return touch_handle_get_rev_date_detail(req);
    case CMD_MYSTERY1:
        return touch_handle_mystery1(req);
    case CMD_MYSTERY2:
        return touch_handle_mystery2(req);
    case CMD_START_AUTO_SCAN:
        return touch_handle_start_auto_scan(req);
    case CMD_BEGIN_WRITE:
        dprintf("Wacca touch: Begin write for side %d\n", req->side);
        return S_OK;
    case CMD_NEXT_WRITE:
        dprintf("Wacca touch: continue write for side %d\n", req->side);
        return S_OK;
    default:
        dprintf("Wacca touch: Unhandled command %02x\n", req->cmd);
        return S_OK;
    }
}

static HRESULT touch_handle_get_rev_date(const struct touch_req *req)
{
    struct touch_resp_get_rev_date resp;
    HRESULT hr;
    uint8_t rev[6] = { 0x31, 0x39, 0x30, 0x35, 0x32, 0x33 };

    dprintf("Wacca Touch%d: Get board rev date\n", req->side);

    resp.cmd = 0xa0;
    memcpy(resp.data, rev, sizeof(rev));
    //resp.data = rev;

    if (req->side == 0) {
        hr = touch_frame_encode(&touch0_uart.readable, &resp, sizeof(resp));
    }
    else {
        hr = touch_frame_encode(&touch1_uart.readable, &resp, sizeof(resp));
    }
    return hr;
}

/* TODO: Very ugly please make better before upstreaming */
static HRESULT touch_handle_startup(const struct touch_req *req)
{
    struct touch_resp_startup resp;
    HRESULT hr;
    uint8_t *rev;

    dprintf("Wacca Touch%d: Startup\n", req->side);

    switch (req->data[2]) {
        case 0x30:
            rev = (uint8_t[80]) { 0x20, 0x20, 0x20, 0x20, 0x30, 0x20, 0x20, 0x20, 0x20, 0x30, 0x20,
            0x20, 0x20, 0x20, 0x31, 0x20, 0x20, 0x20, 0x20, 0x32, 0x20, 0x20, 0x20, 0x20,
            0x33, 0x20, 0x20, 0x20, 0x20, 0x34, 0x20, 0x20, 0x20, 0x20, 0x35, 0x20, 0x20,
            0x20, 0x31, 0x35, 0x20, 0x20, 0x20, 0x31, 0x35, 0x20, 0x20, 0x20, 0x31, 0x35,
            0x20, 0x20, 0x20, 0x31, 0x35, 0x20, 0x20, 0x20, 0x31, 0x35, 0x20, 0x20, 0x20,
            0x31, 0x35, 0x20, 0x20, 0x20, 0x31, 0x31, 0x20, 0x20, 0x20, 0x31, 0x31, 0x20,
            0x20, 0x20, 0x31, 0x31 };
            break;
        case 0x31:
            rev = (uint8_t[80]) { 0x20, 0x20, 0x20, 0x31, 0x31, 0x20, 0x20, 0x20, 0x31, 0x31, 0x20,
            0x20, 0x20, 0x31, 0x31, 0x20, 0x20, 0x31, 0x32, 0x38, 0x20, 0x20, 0x31, 0x30,
            0x33, 0x20, 0x20, 0x31, 0x30, 0x33, 0x20, 0x20, 0x31, 0x31, 0x35, 0x20, 0x20,
            0x31, 0x33, 0x38, 0x20, 0x20, 0x31, 0x32, 0x37, 0x20, 0x20, 0x31, 0x30, 0x33,
            0x20, 0x20, 0x31, 0x30, 0x35, 0x20, 0x20, 0x31, 0x31, 0x31, 0x20, 0x20, 0x31,
            0x32, 0x36, 0x20, 0x20, 0x31, 0x31, 0x33, 0x20, 0x20, 0x20, 0x39, 0x35, 0x20,
            0x20, 0x31, 0x30, 0x30 };
            break;
        case 0x33:
            rev = (uint8_t[80]) { 0x20, 0x20, 0x31, 0x30, 0x31, 0x20, 0x20, 0x31, 0x31, 0x35, 0x20,
            0x20, 0x20, 0x39, 0x38, 0x20, 0x20, 0x20, 0x38, 0x36, 0x20, 0x20, 0x20, 0x37,
            0x36, 0x20, 0x20, 0x20, 0x36, 0x37, 0x20, 0x20, 0x20, 0x36, 0x38, 0x20, 0x20,
            0x20, 0x34, 0x38, 0x20, 0x20, 0x31, 0x31, 0x37, 0x20, 0x20, 0x20, 0x20, 0x30,
            0x20, 0x20, 0x20, 0x38, 0x32, 0x20, 0x20, 0x31, 0x35, 0x34, 0x20, 0x20, 0x20,
            0x20, 0x30, 0x20, 0x20, 0x20, 0x20, 0x36, 0x20, 0x20, 0x20, 0x33, 0x35, 0x20,
            0x20, 0x20, 0x20, 0x34 };
            break;
        default:
            dprintf("Wacca touch: BAD STARTUP REQUEST %2hx\n", req->data[2]);
            return 1;
    }

    memcpy(resp.data, rev, 80 * sizeof(uint8_t));

    if (req->side == 0) {
        hr = touch_frame_encode(&touch0_uart.readable, &resp, sizeof(resp));
    }
    else {
        hr = touch_frame_encode(&touch1_uart.readable, &resp, sizeof(resp));
    }
    return hr;
}

static HRESULT touch_handle_get_rev_date_detail(const struct touch_req *req)
{
    struct touch_resp_get_rev_date_detail resp;
    HRESULT hr;
    uint8_t rev[43] = { 0x31, 0x39, 0x30, 0x35, 0x32, 0x33, 0x52, 0x31,
    0x39, 0x30, 0x35, 0x31, 0x34, 0x31, 0x39, 0x30, 0x35, 0x31, 0x34, 0x31,
    0x39, 0x30, 0x35, 0x31, 0x34, 0x31, 0x39, 0x30, 0x35, 0x31, 0x34, 0x31,
    0x39, 0x30, 0x35, 0x31, 0x34, 0x31, 0x39, 0x30, 0x35, 0x31, 0x34 };

    dprintf("Wacca Touch%d: get rev date detail\n", req->side);

    resp.cmd = 0xa8;
    memcpy(resp.data, rev, sizeof(rev));

    if (req->side == 0) {
        hr = touch_frame_encode(&touch0_uart.readable, &resp, sizeof(resp));
    }
    else {
        hr = touch_frame_encode(&touch1_uart.readable, &resp, sizeof(resp));
    }
    return hr;

}

static HRESULT touch_handle_mystery1(const struct touch_req *req)
{
    struct touch_resp_mystery1 resp;
    HRESULT hr;

    dprintf("Wacca Touch%d: mystery command 1\n", req->side);

    resp.cmd = 0xa2;
    resp.data = 0x3f;

    if (req->side == 0) {
        hr = touch_frame_encode(&touch0_uart.readable, &resp, sizeof(resp));
    }
    else {
        hr = touch_frame_encode(&touch1_uart.readable, &resp, sizeof(resp));
    }
    return hr;
}

static HRESULT touch_handle_mystery2(const struct touch_req *req)
{
    struct touch_resp_mystery2 resp;
    HRESULT hr;

    dprintf("Wacca Touch%d: mystery command 2\n", req->side);

    resp.cmd = 0x94;
    resp.data = 0;

    if (req->side == 0) {
        hr = touch_frame_encode(&touch0_uart.readable, &resp, sizeof(resp));
    }
    else {
        hr = touch_frame_encode(&touch1_uart.readable, &resp, sizeof(resp));
    }
    return hr;
}

static HRESULT touch_handle_start_auto_scan(const struct touch_req *req)
{
    struct touch_resp_start_auto resp;
    HRESULT hr;
    uint8_t data1[24] = { 0 };
    uint8_t data2[9] = { 0x0d, 0x03, 0x02, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00 };

    dprintf("Wacca Touch%d: Start Auto\n", req->side);

    resp.cmd = 0x9c;
    resp.data = 0;
    resp.checksum = 0x49;

    resp.frame.cmd= 0x81;
    resp.frame.count = input_frame_count++;
    memcpy(resp.frame.data1, data1, sizeof(data1));
    memcpy(resp.frame.data2, data2, sizeof(data2));
    resp.frame.checksum = calc_checksum(&resp.frame, sizeof(resp.frame));

    if (req->side == 0) {
        hr = touch_frame_encode(&touch0_uart.readable, &resp, sizeof(resp));
    }
    else {
        hr = touch_frame_encode(&touch1_uart.readable, &resp, sizeof(resp));
    }
    return hr;
}

/* Decodes the response into a struct that's easier to work with. */
static HRESULT touch_frame_decode(struct touch_req *dest, struct iobuf *iobuf, int side)
{
    dest->side = side;
    dest->cmd = iobuf->bytes[0];
    iobuf->pos--;
    dest->data_length = iobuf->pos;

    if (dest->data_length > 0) {
        for (int i = 1; i < dest->data_length; i++) {
            dest->data[i-1] = iobuf->bytes[i];
        }
    }
    iobuf->pos -= dest->data_length;

    return S_OK;
}

/* Encode and send the response.
 * The last byte of every response is a checksum.
 * This checksum is calculated by bitwise XORing
 * every byte in the response, then dropping the MSB.
 * Thanks the CrazyRedMachine for figuring that out!!
 */
static HRESULT touch_frame_encode(struct iobuf *dest, const void *ptr, size_t nbytes)
{
    const uint8_t *src;
    uint8_t checksum = 0;

    src = ptr;

    for (size_t i = 0; i < nbytes; i++) {
        dest->bytes[dest->pos++] = src[i];
        checksum = checksum^(src[i]);
    }

    dest->bytes[dest->pos++] = checksum&0x7f;
    return S_OK;
}

static uint8_t calc_checksum(const void *ptr, size_t nbytes)
{
    const uint8_t *src;
    uint8_t checksum = 0;

    src = ptr;

    for (size_t i = 0; i < nbytes; i++) {
        checksum = checksum^(src[i]);
    }

    return checksum&0x7f;
}
