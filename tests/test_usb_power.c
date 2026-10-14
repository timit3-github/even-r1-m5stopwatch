/* SPDX-License-Identifier: BSL-1.0
 * Copyright (c) 2026 even-r1-esp32s3 contributors. */
#include "r1_usb_power.h"
#include <assert.h>
#include <stdio.h>
static void sample(struct r1_usb_power *s,bool valid,uint16_t mv,int64_t ms) {
 r1_usb_power_update(s,valid,mv,ms,4000,3000,2000);
}
int main(void) {
 struct r1_usb_power s={0};
 sample(&s,true,0,0);assert(s.valid && !s.present && !s.lock_held);
 sample(&s,true,3500,500);assert(!s.present && !s.lock_held);
 sample(&s,true,4000,1000);assert(s.present && s.lock_held);
 sample(&s,true,3500,1500);assert(s.present && s.lock_held); /* Hysteresis. */
 sample(&s,false,0,1600);assert(!s.valid && s.lock_held && s.present);
 sample(&s,true,3000,2000);assert(s.lock_held && s.release_pending);
 sample(&s,true,3000,3000);assert(s.lock_held);
 sample(&s,false,0,4000);assert(s.lock_held && !s.release_pending);
 sample(&s,true,0,5000);assert(s.release_pending);
 sample(&s,true,3500,6000);assert(s.lock_held && !s.release_pending);
 sample(&s,true,0,7000);assert(s.release_pending);
 sample(&s,true,4000,8000);assert(s.present && !s.release_pending);
 sample(&s,true,0,9000);
 sample(&s,true,0,10999);assert(s.lock_held);
 sample(&s,true,0,11000);assert(!s.lock_held && !s.release_pending);
 sample(&s,true,5000,12000);assert(s.lock_held && s.present); /* Reinsert. */
 /* Monotonic 64-bit time crosses the old 32-bit millisecond boundary. */
 sample(&s,true,0,(int64_t)UINT32_MAX-1000);
 sample(&s,true,0,(int64_t)UINT32_MAX+1000);assert(!s.lock_held);
 assert(s.failures==2);
 puts("usb_power: ok");
 return 0;
}
