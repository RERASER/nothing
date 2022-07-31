#pragma once

struct led_data {
   DWORD unitCount;
   uint8_t rgba[480 * 4];
};

HRESULT elisabeth_hook_init();
