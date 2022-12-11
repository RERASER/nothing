#include <windows.h>
#include <stdbool.h>
#include <stdint.h>

#include "cxbhook/network.h"

#include "util/dprintf.h"

HRESULT network_hook_init(struct network_config *cfg)
{
    dprintf("Network: Init\n");
    return S_OK;
}