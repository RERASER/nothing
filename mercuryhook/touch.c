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

const char SYNC_BOARD_VER[6] = "190523";
const char UNIT_BOARD_VER[6] = "190514";

static HRESULT touch_handle_irp(struct irp *irp);
static HRESULT touch0_handle_irp_locked(struct irp *irp);
static HRESULT touch1_handle_irp_locked(struct irp *irp);

static HRESULT touch_req_dispatch(const struct touch_req *req);

static HRESULT touch_frame_decode(struct touch_req *dest, struct iobuf *iobuf, int side);
static uint8_t calc_checksum(const void *ptr, size_t nbytes);

static HRESULT touch_handle_get_sync_board_ver(const struct touch_req *req);
static HRESULT touch_handle_next_read(const struct touch_req *req);
static HRESULT touch_handle_get_unit_board_ver(const struct touch_req *req);
static HRESULT touch_handle_mystery1(const struct touch_req *req);
static HRESULT touch_handle_mystery2(const struct touch_req *req);
static HRESULT touch_handle_start_auto_scan(const struct touch_req *req);
static void touch_res_auto_scan(const bool *state);

uint8_t input_frame_count_0 = 0x7b;
uint8_t input_frame_count_1 = 0x7b;
bool touch0_auto = false;
bool touch1_auto = false;

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

    if (!cfg->enable) {
        return S_FALSE;
    }

    InitializeCriticalSection(&touch0_lock);
    InitializeCriticalSection(&touch1_lock);
    dprintf("Wacca touch: Init\n");

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
    case CMD_GET_SYNC_BOARD_VER:
        return touch_handle_get_sync_board_ver(req);
    case CMD_NEXT_READ:
        return touch_handle_next_read(req);
    case CMD_GET_UNIT_BOARD_VER:
        return touch_handle_get_unit_board_ver(req);
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

static HRESULT touch_handle_get_sync_board_ver(const struct touch_req *req)
{
    struct touch_resp_get_sync_board_ver resp;
    HRESULT hr;

    dprintf("Wacca Touch%d: Get sync board version\n", req->side);

    resp.cmd = 0xa0;
    memcpy(resp.version, SYNC_BOARD_VER, sizeof(SYNC_BOARD_VER));
    resp.checksum = 0;
    resp.checksum = calc_checksum(&resp, sizeof(resp));

    if (req->side == 0) {
        hr = iobuf_write(&touch0_uart.readable, &resp, sizeof(resp));
    }
    else {
        hr = iobuf_write(&touch1_uart.readable, &resp, sizeof(resp));
    }

    return hr;
}

static HRESULT touch_handle_next_read(const struct touch_req *req)
{
    struct touch_resp_startup resp;
    HRESULT hr;
    char *rev;

    dprintf("Wacca Touch%d: Read section %2hx\n", req->side, req->data[2]);


    switch (req->data[2]) {
        // These can be found in the config file
        case 0x30:
            rev = "    0    0    1    2    3    4    5   15   15   15   15   15   15   11   11   11";
            break;
        case 0x31:
            rev = "   11   11   11  128  103  103  115  138  127  103  105  111  126  113   95  100";
            break;
        case 0x33:
           rev = "  101  115   98   86   76   67   68   48  117    0   82  154    0    6   35    4";
            break;
        default:
            dprintf("Wacca touch: BAD READ REQUEST %2hx\n", req->data[2]);
            return 1;
    }

    memcpy(resp.data, rev, 80 * sizeof(char));
    resp.checksum = 0;
    resp.checksum = calc_checksum(&resp, sizeof(resp));

    if (req->side == 0) {
        hr = iobuf_write(&touch0_uart.readable, &resp, sizeof(resp));
    }
    else {
        hr = iobuf_write(&touch1_uart.readable, &resp, sizeof(resp));
    }
    return hr;
}

static HRESULT touch_handle_get_unit_board_ver(const struct touch_req *req)
{
    struct touch_resp_get_unit_board_ver resp;
    HRESULT hr;

    dprintf("Wacca Touch%d: get unit board version\n", req->side);

    memset(resp.version, 0, sizeof(resp.version));
    memcpy(resp.version, SYNC_BOARD_VER, sizeof(SYNC_BOARD_VER));

    for (int i = 0; i < 6; i++ )
        memcpy(&resp.version[7 + (6 * i)], UNIT_BOARD_VER, sizeof(UNIT_BOARD_VER));

    resp.cmd = 0xa8;
    resp.checksum = 0;

    if (req->side == 0) {        
        resp.version[6] = 'R';
        resp.checksum = calc_checksum(&resp, sizeof(resp));

        #if 0
        for (int i = 0; i < sizeof(resp.version); i++) {
            dprintf("0x%02x ", resp.version[i]);
        }
        dprintf("\n");
        #endif

        hr = iobuf_write(&touch0_uart.readable, &resp, sizeof(resp));
    }
    else {
        resp.version[6] = 'L';
        resp.checksum = calc_checksum(&resp, sizeof(resp));

        #if 0
        for (int i = 0; i < sizeof(resp.version); i++) {
            dprintf("0x%02x ", resp.version[i]);
        }
        dprintf("\n");
        #endif

        hr = iobuf_write(&touch1_uart.readable, &resp, sizeof(resp));
    }

    return hr;
}

static HRESULT touch_handle_mystery1(const struct touch_req *req)
{
    struct touch_resp_mystery1 resp;
    HRESULT hr;

    dprintf("Wacca Touch%d: Command A2\n", req->side);

    resp.cmd = 0xa2;
    resp.data = 0x3f;
    resp.checksum = 0;
    resp.checksum = calc_checksum(&resp, sizeof(resp));

    if (req->side == 0) {
        hr = iobuf_write(&touch0_uart.readable, &resp, sizeof(resp));
    }
    else {
        hr = iobuf_write(&touch1_uart.readable, &resp, sizeof(resp));
    }
    return hr;
}

static HRESULT touch_handle_mystery2(const struct touch_req *req)
{
    struct touch_resp_mystery2 resp;
    HRESULT hr;

    dprintf("Wacca Touch%d: Command 94\n", req->side);

    resp.cmd = 0x94;
    resp.data = 0;
    resp.checksum = 0;
    resp.checksum = calc_checksum(&resp, sizeof(resp));

    if (req->side == 0) {
        hr = iobuf_write(&touch0_uart.readable, &resp, sizeof(resp));
    }    
    else {
        hr = iobuf_write(&touch1_uart.readable, &resp, sizeof(resp));
    }
    return hr;
}

static HRESULT touch_handle_start_auto_scan(const struct touch_req *req)
{
    struct touch_resp_start_auto resp;
    HRESULT hr;
    uint8_t data1[24] = { 0 };
    // Unsure what this does. It seems to change every request on a real board,
    // but the game doesn't seem to mind that it's the same
    uint8_t data2[9] = { 0x0d, 0x03, 0x02, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00 };

    dprintf("Wacca Touch%d: Start Auto", req->side);

    #if 0
    for (int i = 0; i < req->data_length; i++)
        dprintf("0x%02x ", req->data[i]);
    #endif
    dprintf("\n");

    resp.cmd = 0x9c;
    resp.data = 0;
    resp.checksum = 0x49;

    resp.frame.cmd= 0x81;
    memcpy(resp.frame.data1, data1, sizeof(data1));
    memcpy(resp.frame.data2, data2, sizeof(data2));
    resp.frame.checksum = 0;
    resp.frame.checksum = calc_checksum(&resp.frame, sizeof(resp.frame));

    if (req->side == 0) {
        resp.frame.count = input_frame_count_0++;
        hr = iobuf_write(&touch0_uart.readable, &resp, sizeof(resp));
        touch0_auto = true;
    }
    else {
        resp.frame.count = input_frame_count_1++;
        hr = iobuf_write(&touch1_uart.readable, &resp, sizeof(resp));
        touch1_auto = true;
    }

    mercury_dll.touch_start(touch_res_auto_scan);
    return hr;
}

static void touch_res_auto_scan(const bool *state)
{
    struct touch_input_frame frame0;
    struct touch_input_frame frame1;
    uint8_t dataR[24] = { 0 };
    uint8_t dataL[24] = { 0 };
    // this changes every input on a real board but
    // the game doesn't seem to care about it...
    uint8_t data2[9] = { 0x0d, 0x03, 0x02, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00 };
    uint8_t counter = 0;

    frame0.cmd = 0x81;
    frame0.count = input_frame_count_0++;
    input_frame_count_0 %= 0x7f;

    frame1.cmd = 0x81;
    frame1.count = input_frame_count_1++;
    input_frame_count_1 %= 0x7f;

    for (int i = 0; i < 24; i++) {
        for (int j = 0; j < 5; j++) {
            if (state[counter]) {
                dataR[i] |= (1 << j);
            }
            if (state[counter+120]) {
                dataL[i] |= (1 << j);
            }
            counter++;
        }
    }
    
    memcpy(frame0.data1, dataR, sizeof(dataR));
    memcpy(frame0.data2, data2, sizeof(data2));

    memcpy(frame1.data1, dataL, sizeof(dataL));
    memcpy(frame1.data2, data2, sizeof(data2));
    
    frame0.checksum = 0;
    frame0.checksum = calc_checksum(&frame0, sizeof(frame0));

    frame1.checksum = 0;
    frame1.checksum = calc_checksum(&frame1, sizeof(frame1));

    if (touch0_auto) {
        //dprintf("Wacca touch: Touch0 auto frame #%2hx sent\n", frame0.count);
        EnterCriticalSection(&touch0_lock);
        iobuf_write(&touch0_uart.readable, &frame0, sizeof(frame0));
        LeaveCriticalSection(&touch0_lock);
    }

    if (touch1_auto) {
        //dprintf("Wacca touch: Touch1 auto frame #%2hx sent\n", frame0.count);
        EnterCriticalSection(&touch1_lock);
        iobuf_write(&touch1_uart.readable, &frame1, sizeof(frame1));
        LeaveCriticalSection(&touch1_lock);
    }
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

/* The last byte of every response is a checksum.
 * This checksum is calculated by bitwise XORing
 * every byte in the response, then dropping the MSB.
 * Thanks the CrazyRedMachine for figuring that out!!
 */
static uint8_t calc_checksum(const void *ptr, size_t nbytes)
{
    const uint8_t *src;
    uint8_t checksum = 0;

    src = ptr;

    for (size_t i = 0; i < nbytes; i++) {
        //dprintf("Wacca touch: Calculating %2hx\n", src[i]);
        checksum = checksum^(src[i]);
    }
    //dprintf("Wacca touch: Checksum is %2hx\n", checksum&0x7f);
    return checksum&0x7f;
}
