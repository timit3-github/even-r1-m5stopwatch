/* SPDX-License-Identifier: BSL-1.0
 * Copyright (c) 2026 even-r1-esp32s3 contributors. */
#include "r1_idle.h"
void r1_idle_init(struct r1_idle *s,uint32_t now) {
 *s=(struct r1_idle){.last_activity_ms=now};
}
bool r1_idle_update(struct r1_idle *s,uint32_t now,uint32_t timeout,
                    bool button,bool touch,bool busy) {
 if(!timeout) {s->waiting=false;s->last_activity_ms=now;return false;}
 if(button || (touch && !s->waiting)) {
  if(s->waiting) s->resumes++;
  s->waiting=false;s->last_activity_ms=now;
 } else if(!s->waiting && !busy && (uint32_t)(now-s->last_activity_ms)>=timeout) {
  s->waiting=true;s->entries++;
 }
 return s->waiting;
}
