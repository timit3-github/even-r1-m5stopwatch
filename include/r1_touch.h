/* SPDX-License-Identifier: BSL-1.0
 * Copyright (c) 2026 even-r1-esp32s3 contributors. */
#pragma once
#include "r1_inputs.h"
#include <stddef.h>
struct r1_touch_sample {bool valid,down,position;uint16_t x,y;int64_t ms;uint32_t epoch;};
bool r1_touch_decode(const uint8_t *bytes,size_t n,struct r1_touch_sample *sample);
struct r1_touch_gesture {
 struct r1_button button;
 bool contact,moved,swiped;
 int start_x,start_y;
 unsigned swipe_direction;
 uint32_t swipe_ms;
};
enum {R1_INPUT_UP=32,R1_INPUT_DOWN=64,R1_INPUT_REPEAT=128};
void r1_touch_gesture_init(struct r1_touch_gesture *s,bool down,uint32_t now);
unsigned r1_touch_gesture_update(struct r1_touch_gesture *s,bool down,int x,int y,
 uint32_t now,uint32_t hold_ms,uint32_t double_ms,uint32_t followup_ms,
 unsigned slop,unsigned swipe,uint32_t repeat_ms);
void r1_touch_hw_init(void);
/* Pause reads, not power. Epochs discard in-flight reads across pause/resume. */
void r1_touch_hw_set_active(bool active);
/* Discard samples captured before a connection/input reset, even if active. */
void r1_touch_hw_reset(void);
bool r1_touch_hw_pop(struct r1_touch_sample *sample);
struct r1_touch_sample r1_touch_hw_get(void);
bool r1_touch_hw_overflow(void);
