/* SPDX-License-Identifier: BSL-1.0
 * Copyright (c) 2026 even-r1-esp32s3 contributors. */
#pragma once
#include <stdbool.h>
#include <stdint.h>
enum {R1_SOURCE_G1=1,R1_SOURCE_NAV=2,R1_SOURCE_TOUCH=4,R1_SOURCE_ALL=7};
struct r1_pending_input {char command;uint8_t source;bool repeat;};
struct r1_input_queue {struct r1_pending_input items[8];unsigned head,count;};
bool r1_input_queue_push(struct r1_input_queue *q,struct r1_pending_input input);
const struct r1_pending_input *r1_input_queue_peek(const struct r1_input_queue *q);
void r1_input_queue_pop(struct r1_input_queue *q);
void r1_input_queue_cancel(struct r1_input_queue *q,unsigned sources,bool repeats_only);
