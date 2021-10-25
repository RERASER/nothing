#pragma once

#include <windows.h>

#include "hooklib/gfx/gfx.h"

void gfx_d3d9_hook_init(const struct gfx_config *cfg, HINSTANCE self);
