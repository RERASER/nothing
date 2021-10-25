#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "hooklib/dvd.h"
#include "hooklib/gfx/gfx.h"

void dvd_config_load(struct dvd_config *cfg, const wchar_t *filename);
void gfx_config_load(struct gfx_config *cfg, const wchar_t *filename);
