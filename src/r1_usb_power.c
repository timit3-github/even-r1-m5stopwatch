/* SPDX-License-Identifier: BSL-1.0
 * Copyright (c) 2026 even-r1-esp32s3 contributors. */
#include "r1_usb_power.h"
void r1_usb_power_update(struct r1_usb_power *s,bool valid,
                         uint16_t mv,int64_t now_ms,uint16_t on_mv,
                         uint16_t off_mv,uint32_t release_ms) {
 s->valid=valid;
 if(!valid) {
  s->failures++;
  s->release_pending=false; /* Failed reads never establish removal. */
  return;
 }
 s->millivolts=mv;s->sample_ms=now_ms;
 if(mv>=on_mv) s->present=true;
 else if(mv<=off_mv) s->present=false;
 if(s->present) {s->lock_held=true;s->release_pending=false;}
 else if(!s->lock_held || mv>off_mv) s->release_pending=false;
 else if(!s->release_pending) {
  s->release_pending=true;s->low_since_ms=now_ms;
 } else if(now_ms-s->low_since_ms>=release_ms) {
  s->lock_held=false;s->release_pending=false;
 }
}
