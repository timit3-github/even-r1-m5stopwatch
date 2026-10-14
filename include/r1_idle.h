/* SPDX-License-Identifier: BSL-1.0
 * Copyright (c) 2026 even-r1-esp32s3 contributors. */
#pragma once
#include <stdbool.h>
#include <stdint.h>

/* Host-owned user inactivity state, distinct from CPU/BLE sleep. */
struct r1_idle { bool waiting; uint32_t last_activity_ms, entries, resumes; };
void r1_idle_init(struct r1_idle *s,uint32_t now);
/* A physical button can leave waiting. Touch can extend active time only.
 * busy prevents entry during an unfinished gesture or queued operation. */
bool r1_idle_update(struct r1_idle *s,uint32_t now,uint32_t timeout,
                    bool button,bool touch,bool busy);
