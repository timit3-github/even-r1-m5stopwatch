/* SPDX-License-Identifier: BSL-1.0
 * Copyright (c) 2026 even-r1-esp32s3 contributors. */
#pragma once
#include <stdbool.h>
#include <stdint.h>

struct r1_encoder { uint8_t phase; int8_t partial; };
void r1_encoder_init(struct r1_encoder *s,uint8_t phase);
/* phase=(A<<1)|B; +1 for 3->1->0->2->3, -1 for reverse. */
int r1_encoder_update(struct r1_encoder *s,uint8_t phase,unsigned edges);

enum { R1_INPUT_CLICK=1, R1_INPUT_HOLD=2, R1_INPUT_RELEASE=4,
       R1_INPUT_DOUBLE=8, R1_INPUT_TAP_HOLD=16 };
struct r1_button {
 bool raw,stable,armed,held,pending_click,followup;
 uint32_t changed_ms,pressed_ms,tap_ms,active_hold_ms,nav_repeat_ms;
};
void r1_button_init(struct r1_button *s,bool down,uint32_t now);
unsigned r1_button_update(struct r1_button *s,bool down,uint32_t now,
                         uint32_t debounce_ms,uint32_t hold_ms,
                         uint32_t double_ms,uint32_t followup_hold_ms);

enum { R1_NAV_NONE=0, R1_NAV_DOWN=1, R1_NAV_UP=2 };
/* Short release=down, hold=up; repeat_ms=0 disables further ups.
 * Repeats stop on raw release; delayed polls never emit a catch-up burst. */
unsigned r1_nav_button_update(struct r1_button *s,bool down,uint32_t now,
                             uint32_t debounce_ms,uint32_t hold_ms,uint32_t repeat_ms);
