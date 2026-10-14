/* SPDX-License-Identifier: BSL-1.0
 * Copyright (c) 2026 even-r1-esp32s3 contributors. */
#pragma once
#include <stdbool.h>
void r1_haptic_init(void);
/* Asynchronous feedback only for a manual toggle, never for NVS completion. */
void r1_haptic_touch_toggle(bool enabled);
