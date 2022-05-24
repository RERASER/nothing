#include <windows.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#include "ringsec.h"

HRESULT ringsec_hook_init(
        const struct ringsec_config *cfg,
        const char *game_id,
        const char *platform_id)
{
    HRESULT hr;

    assert(cfg != NULL);
    assert(game_id != NULL && strlen(game_id) == sizeof(cfg->game_id));
    assert(platform_id != NULL && strlen(platform_id) == sizeof(cfg->platform_id));

    if (!cfg->enable) {
        return S_FALSE;
    }
}