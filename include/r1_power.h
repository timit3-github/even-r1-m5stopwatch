/* SPDX-License-Identifier: BSL-1.0
 * Copyright (c) 2026 even-r1-esp32s3 contributors. */
#pragma once
#include <stdbool.h>
/* Atomic snapshot for measurement tasks; host owns transitions. */
bool r1_power_waiting(void);
