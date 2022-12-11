#pragma once

#include <windows.h>
#include <stdbool.h>
#include <stdint.h>

uint16_t cxb_io_get_api_version(void);

HRESULT cxb_io_revio_init(void);

void cxb_io_revio_poll(uint16_t *opbtn);

void cxb_io_revio_get_coins(long *coins);

void cxb_io_revio_set_coins(int coins);

HRESULT cxb_io_led_init(void);

void cxb_io_led_update(int id, int color);