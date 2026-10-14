/* SPDX-License-Identifier: BSL-1.0
 * Copyright (c) 2026 even-r1-esp32s3 contributors. */
#include "r1_idle.h"
#include "r1_inputs.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
 struct r1_idle s;r1_idle_init(&s,0);
 assert(!r1_idle_update(&s,59999,60000,false,false,false));
 assert(!r1_idle_update(&s,60000,60000,false,false,true));
 assert(r1_idle_update(&s,60001,60000,false,false,false));
 /* BLE work and a stale/real touch never leave the button-only wait. */
 assert(r1_idle_update(&s,61000,60000,false,true,true));
 assert(s.entries==1 && s.resumes==0);
 assert(!r1_idle_update(&s,62000,60000,true,false,false));
 assert(s.resumes==1);
 assert(!r1_idle_update(&s,120000,60000,false,true,false));
 assert(!r1_idle_update(&s,179999,60000,false,false,false));
 assert(r1_idle_update(&s,180000,60000,false,false,false));
 assert(!r1_idle_update(&s,180001,0,false,false,false));
 r1_idle_init(&s,UINT32_MAX-20);
 assert(!r1_idle_update(&s,28,50,false,false,false));
 assert(r1_idle_update(&s,29,50,false,false,false));
 /* A first wake press uses existing armed state, preserving all gestures. */
 struct r1_button b;r1_button_init(&b,false,0);
 r1_idle_init(&s,0);assert(r1_idle_update(&s,60000,60000,false,false,false));
 assert(!r1_idle_update(&s,61000,60000,true,false,false));
 assert(!r1_button_update(&b,true,61000,30,700,300,200));
 assert(!r1_button_update(&b,true,61030,30,700,300,200));
 assert(!r1_button_update(&b,false,61100,30,700,300,200));
 assert(!r1_button_update(&b,false,61130,30,700,300,200));
 assert(r1_button_update(&b,false,61430,30,700,300,200)==R1_INPUT_CLICK);
 r1_button_init(&b,false,0);
 assert(!r1_nav_button_update(&b,true,61000,30,700,0,700,500));
 assert(!r1_nav_button_update(&b,true,61030,30,700,0,700,500));
 assert(r1_nav_button_update(&b,true,61730,30,700,0,700,500)==R1_NAV_UP);
 assert(r1_nav_button_update(&b,true,62230,30,700,0,700,500)==R1_NAV_UP);
 assert(!r1_nav_button_update(&b,false,62240,30,700,0,700,500));
 assert(!r1_nav_button_update(&b,false,62270,30,700,0,700,500));
 puts("idle tests passed: timeout, unfinished work, button-only resume, first G1/G2 press, clock wrap");
}
