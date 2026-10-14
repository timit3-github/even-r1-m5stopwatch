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
 bool raw,stable,armed,held,pending_click,followup,nav_tap_valid;
 uint32_t changed_ms,pressed_ms,tap_ms,active_hold_ms,nav_repeat_ms;
 uint32_t nav_tap_ms;
 unsigned nav_direction;
};
void r1_button_init(struct r1_button *s,bool down,uint32_t now);
unsigned r1_button_update(struct r1_button *s,bool down,uint32_t now,
                         uint32_t debounce_ms,uint32_t hold_ms,
                         uint32_t double_ms,uint32_t followup_hold_ms);

enum { R1_NAV_NONE=0, R1_NAV_DOWN=1, R1_NAV_UP=2 };
/* Short release=down, hold=up, short then hold=down.
 * double_ms=0 disables the short-then-hold shortcut; repeat_ms=0 disables repeats.
 * Repeats stop on raw release; delayed polls never emit a catch-up burst. */
unsigned r1_nav_button_update(struct r1_button *s,bool down,uint32_t now,
 uint32_t debounce_ms,uint32_t hold_ms,uint32_t double_ms,
 uint32_t followup_hold_ms,uint32_t repeat_ms);

struct r1_chord {
 unsigned raw;
 bool blocked,qualified,toggled;
 uint32_t changed_ms,first_ms,both_ms;
};
enum {R1_CHORD_BLOCK=1,R1_CHORD_START=2,R1_CHORD_TOGGLE=4};
void r1_chord_init(struct r1_chord *s,unsigned raw,uint32_t now);
/* raw bit0=G1, bit1=G2. A candidate owns both buttons until debounced release. */
unsigned r1_chord_update(struct r1_chord *s,unsigned raw,uint32_t now,
 uint32_t debounce_ms,uint32_t window_ms,uint32_t hold_ms);
