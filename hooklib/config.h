#pragma once

#include <stddef.h>

#include "hooklib/dvd.h"

void dvd_config_load(struct dvd_config *cfg, const wchar_t *filename);
