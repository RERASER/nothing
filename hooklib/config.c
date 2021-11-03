#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "hooklib/config.h"
#include "hooklib/dvd.h"

void dvd_config_load(struct dvd_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"dvd", L"enable", 1, filename);
}
