/* SPDX-License-Identifier: BSL-1.0
 * Copyright (c) 2026 even-r1-esp32s3 contributors. */
#include "r1_inputs.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

int main(void) {
 struct r1_encoder e;
 r1_encoder_init(&e,3);
 const uint8_t forward[]={1,0,2,3},reverse[]={2,0,1,3};
 for(unsigned i=0;i<4;i++) assert(r1_encoder_update(&e,forward[i],4)==(i==3?1:0));
 for(unsigned i=0;i<4;i++) assert(r1_encoder_update(&e,reverse[i],4)==(i==3?-1:0));
 /* Contact bounce and reversing halfway must not fabricate a detent. */
 const uint8_t bounce[]={1,3,1,0,1,0,2,3};
 for(unsigned i=0;i<8;i++) assert(r1_encoder_update(&e,bounce[i],4)==(i==7?1:0));
 const uint8_t partial[]={1,0,1,3};
 for(unsigned i=0;i<4;i++) assert(!r1_encoder_update(&e,partial[i],4));
 assert(!r1_encoder_update(&e,0,4)); /* impossible diagonal resets */
 assert(!r1_encoder_update(&e,0,4));
 assert(!r1_encoder_update(&e,2,4));assert(!r1_encoder_update(&e,3,4));
 assert(!r1_encoder_update(&e,1,4));assert(r1_encoder_update(&e,0,4)==1);
 r1_encoder_init(&e,3);assert(!r1_encoder_update(&e,1,2));assert(r1_encoder_update(&e,0,2)==1);
 struct r1_button b;r1_button_init(&b,false,0);
 assert(!r1_button_update(&b,true,10,30,700,0,200));
 assert(!r1_button_update(&b,false,20,30,700,0,200));
 assert(!r1_button_update(&b,false,60,30,700,0,200)); /* bounce only */
 assert(!r1_button_update(&b,true,100,30,700,0,200));
 assert(!r1_button_update(&b,true,129,30,700,0,200));
 assert(!r1_button_update(&b,true,130,30,700,0,200));
 assert(!r1_button_update(&b,false,200,30,700,0,200));
 assert(r1_button_update(&b,false,230,30,700,0,200)==R1_INPUT_CLICK);
 assert(!r1_button_update(&b,false,250,30,700,0,200));
 assert(!r1_button_update(&b,true,300,30,700,0,200));
 assert(!r1_button_update(&b,true,330,30,700,0,200));
 assert(!r1_button_update(&b,true,1029,30,700,0,200));
 assert(r1_button_update(&b,true,1030,30,700,0,200)==R1_INPUT_HOLD);
 assert(!r1_button_update(&b,true,1200,30,700,0,200));
 assert(!r1_button_update(&b,false,1300,30,700,0,200));
 assert(r1_button_update(&b,false,1330,30,700,0,200)==R1_INPUT_RELEASE);
 assert(!r1_button_update(&b,false,1400,30,700,0,200));
 r1_button_init(&b,true,0);assert(!r1_button_update(&b,true,1000,30,700,0,200));
 assert(!r1_button_update(&b,false,1010,30,700,0,200));assert(!r1_button_update(&b,false,1040,30,700,0,200));
 /* Timer wrap and late poll spanning the hold threshold. */
 r1_button_init(&b,false,UINT32_MAX-100);
 assert(!r1_button_update(&b,true,UINT32_MAX-80,30,700,0,200));
 assert(!r1_button_update(&b,true,UINT32_MAX-50,30,700,0,200));
 assert(r1_button_update(&b,true,649,30,700,0,200)==R1_INPUT_HOLD);
 assert(!r1_button_update(&b,false,660,30,700,0,200));
 assert(r1_button_update(&b,false,690,30,700,0,200)==R1_INPUT_RELEASE);
 r1_button_init(&b,false,0);assert(!r1_button_update(&b,true,10,30,700,0,200));
 assert(!r1_button_update(&b,true,40,30,700,0,200));
 assert(!r1_button_update(&b,false,900,30,700,0,200));
 assert(r1_button_update(&b,false,930,30,700,0,200)==(R1_INPUT_HOLD|R1_INPUT_RELEASE));
 /* New gestures: delayed single, double, tap then hold and window boundaries. */
 r1_button_init(&b,false,0);
 assert(!r1_button_update(&b,true,10,30,700,300,200));
 assert(!r1_button_update(&b,true,40,30,700,300,200));
 assert(!r1_button_update(&b,false,100,30,700,300,200));
 assert(!r1_button_update(&b,false,130,30,700,300,200));
 assert(!r1_button_update(&b,false,429,30,700,300,200));
 assert(r1_button_update(&b,false,430,30,700,300,200)==R1_INPUT_CLICK);
 assert(!r1_button_update(&b,false,450,30,700,300,200));

 /* Two quick releases produce only DOUBLE. */
 r1_button_init(&b,false,0);
 assert(!r1_button_update(&b,true,10,30,700,300,200));
 assert(!r1_button_update(&b,true,40,30,700,300,200));
 assert(!r1_button_update(&b,false,100,30,700,300,200));
 assert(!r1_button_update(&b,false,130,30,700,300,200));
 assert(!r1_button_update(&b,true,200,30,700,300,200));
 assert(!r1_button_update(&b,true,230,30,700,300,200));
 assert(!r1_button_update(&b,false,280,30,700,300,200));
 assert(r1_button_update(&b,false,310,30,700,300,200)==R1_INPUT_DOUBLE);
 assert(!r1_button_update(&b,false,800,30,700,300,200));

 /* Tap then hold emits one dedicated event, without CLICK or plain HOLD. */
 r1_button_init(&b,false,0);
 assert(!r1_button_update(&b,true,10,30,700,300,200));
 assert(!r1_button_update(&b,true,40,30,700,300,200));
 assert(!r1_button_update(&b,false,100,30,700,300,200));
 assert(!r1_button_update(&b,false,130,30,700,300,200));
 assert(!r1_button_update(&b,true,200,30,700,300,200));
 assert(!r1_button_update(&b,true,230,30,700,300,200));
 assert(!r1_button_update(&b,true,429,30,700,300,200));
 assert(r1_button_update(&b,true,430,30,700,300,200)==R1_INPUT_TAP_HOLD);
 assert(!r1_button_update(&b,true,1000,30,700,300,200));
 assert(!r1_button_update(&b,false,1100,30,700,300,200));
 assert(r1_button_update(&b,false,1130,30,700,300,200)==R1_INPUT_RELEASE);
 assert(!r1_button_update(&b,false,1600,30,700,300,200));

 /* Raw second press at the deadline survives its debounce. */
 r1_button_init(&b,false,0);
 assert(!r1_button_update(&b,true,10,30,700,300,200));
 assert(!r1_button_update(&b,true,40,30,700,300,200));
 assert(!r1_button_update(&b,false,100,30,700,300,200));
 assert(!r1_button_update(&b,false,130,30,700,300,200));
 assert(!r1_button_update(&b,true,430,30,700,300,200));
 assert(!r1_button_update(&b,true,460,30,700,300,200));
 assert(r1_button_update(&b,true,660,30,700,300,200)==R1_INPUT_TAP_HOLD);

 /* Too late: flush first tap and retain standalone 700ms hold. */
 r1_button_init(&b,false,0);
 assert(!r1_button_update(&b,true,10,30,700,300,200));
 assert(!r1_button_update(&b,true,40,30,700,300,200));
 assert(!r1_button_update(&b,false,100,30,700,300,200));
 assert(!r1_button_update(&b,false,130,30,700,300,200));
 assert(!r1_button_update(&b,true,431,30,700,300,200));
 assert(r1_button_update(&b,true,461,30,700,300,200)==R1_INPUT_CLICK);
 assert(!r1_button_update(&b,true,1160,30,700,300,200));
 assert(r1_button_update(&b,true,1161,30,700,300,200)==R1_INPUT_HOLD);

 /* Late release poll preserves dedicated tap-hold and release. */
 r1_button_init(&b,false,0);
 assert(!r1_button_update(&b,true,10,30,700,300,200));
 assert(!r1_button_update(&b,true,40,30,700,300,200));
 assert(!r1_button_update(&b,false,100,30,700,300,200));
 assert(!r1_button_update(&b,false,130,30,700,300,200));
 assert(!r1_button_update(&b,true,200,30,700,300,200));
 assert(!r1_button_update(&b,true,230,30,700,300,200));
 assert(!r1_button_update(&b,false,500,30,700,300,200));
 assert(r1_button_update(&b,false,530,30,700,300,200)==(R1_INPUT_TAP_HOLD|R1_INPUT_RELEASE));

 /* Pending tap and second press survive uint32 timer wrap. */
 r1_button_init(&b,false,UINT32_MAX-200);
 assert(!r1_button_update(&b,true,UINT32_MAX-180,30,700,300,200));
 assert(!r1_button_update(&b,true,UINT32_MAX-150,30,700,300,200));
 assert(!r1_button_update(&b,false,UINT32_MAX-100,30,700,300,200));
 assert(!r1_button_update(&b,false,UINT32_MAX-70,30,700,300,200));
 assert(!r1_button_update(&b,true,20,30,700,300,200));
 assert(!r1_button_update(&b,true,50,30,700,300,200));
 assert(!r1_button_update(&b,false,100,30,700,300,200));
 assert(r1_button_update(&b,false,130,30,700,300,200)==R1_INPUT_DOUBLE);

 /* StopWatch navigation: each short click is down; holding is only one up. */
 struct r1_button nav;r1_button_init(&nav,false,0);
 assert(!r1_nav_button_update(&nav,true,10,30,700,0));
 assert(!r1_nav_button_update(&nav,true,40,30,700,0));
 assert(!r1_nav_button_update(&nav,false,100,30,700,0));
 assert(r1_nav_button_update(&nav,false,130,30,700,0)==R1_NAV_DOWN);
 assert(!r1_nav_button_update(&nav,true,150,30,700,0));
 assert(!r1_nav_button_update(&nav,true,180,30,700,0));
 assert(!r1_nav_button_update(&nav,false,220,30,700,0));
 assert(r1_nav_button_update(&nav,false,250,30,700,0)==R1_NAV_DOWN);
 assert(!r1_nav_button_update(&nav,true,300,30,700,0));
 assert(!r1_nav_button_update(&nav,true,330,30,700,0));
 assert(!r1_nav_button_update(&nav,true,1029,30,700,0));
 assert(r1_nav_button_update(&nav,true,1030,30,700,0)==R1_NAV_UP);
 assert(!r1_nav_button_update(&nav,true,1200,30,700,0));
 assert(!r1_nav_button_update(&nav,false,1500,30,700,0));
 assert(!r1_nav_button_update(&nav,false,1530,30,700,0));
 assert(!r1_nav_button_update(&nav,false,1600,30,700,0));
 /* Release debounce must not convert a 699ms press into a long press. */
 r1_button_init(&nav,false,0);
 assert(!r1_nav_button_update(&nav,true,10,30,700,0));
 assert(!r1_nav_button_update(&nav,true,40,30,700,0));
 assert(!r1_nav_button_update(&nav,false,739,30,700,0));
 assert(r1_nav_button_update(&nav,false,769,30,700,0)==R1_NAV_DOWN);
 /* At 700ms, even a late release poll sends up without down. */
 r1_button_init(&nav,false,0);
 assert(!r1_nav_button_update(&nav,true,10,30,700,0));
 assert(!r1_nav_button_update(&nav,true,40,30,700,0));
 assert(!r1_nav_button_update(&nav,false,740,30,700,0));
 assert(r1_nav_button_update(&nav,false,770,30,700,0)==R1_NAV_UP);
 assert(!r1_nav_button_update(&nav,false,800,30,700,0));
 /* GPIO1 pending tap must survive unrelated GPIO2 navigation. */
 r1_button_init(&b,false,0);r1_button_init(&nav,false,0);
 assert(!r1_button_update(&b,true,10,30,700,300,200));
 assert(!r1_button_update(&b,true,40,30,700,300,200));
 assert(!r1_button_update(&b,false,100,30,700,300,200));
 assert(!r1_button_update(&b,false,130,30,700,300,200));
 assert(!r1_nav_button_update(&nav,true,200,30,700,0));
 assert(!r1_nav_button_update(&nav,true,230,30,700,0));
 assert(!r1_nav_button_update(&nav,false,300,30,700,0));
 assert(r1_nav_button_update(&nav,false,330,30,700,0)==R1_NAV_DOWN);
 assert(r1_button_update(&b,false,430,30,700,300,200)==R1_INPUT_CLICK);
 r1_button_init(&nav,true,0);
 assert(!r1_nav_button_update(&nav,true,1000,30,700,0));
 assert(!r1_nav_button_update(&nav,false,1010,30,700,0));
 assert(!r1_nav_button_update(&nav,false,1040,30,700,0));
 /* Repeat begins 500ms after first hold, stops on raw release, no catch-up. */
 r1_button_init(&nav,false,0);
 assert(!r1_nav_button_update(&nav,true,10,30,700,500));
 assert(!r1_nav_button_update(&nav,true,40,30,700,500));
 assert(!r1_nav_button_update(&nav,true,739,30,700,500));
 assert(r1_nav_button_update(&nav,true,740,30,700,500)==R1_NAV_UP);
 assert(!r1_nav_button_update(&nav,true,1239,30,700,500));
 assert(r1_nav_button_update(&nav,true,1240,30,700,500)==R1_NAV_UP);
 assert(!r1_nav_button_update(&nav,true,1241,30,700,500));
 assert(r1_nav_button_update(&nav,true,1740,30,700,500)==R1_NAV_UP);
 assert(r1_nav_button_update(&nav,true,4000,30,700,500)==R1_NAV_UP);
 assert(!r1_nav_button_update(&nav,true,4001,30,700,500));
 assert(!r1_nav_button_update(&nav,true,4499,30,700,500));
 assert(!r1_nav_button_update(&nav,false,4500,30,700,500));
 /* Release contact bounce cannot cause a due repeat. */
 assert(!r1_nav_button_update(&nav,true,4510,30,700,500));
 assert(!r1_nav_button_update(&nav,false,4520,30,700,500));
 assert(!r1_nav_button_update(&nav,false,4550,30,700,500));
 assert(!r1_nav_button_update(&nav,false,5000,30,700,500));
 /* A new press starts with the normal hold delay, not an old repeat deadline. */
 assert(!r1_nav_button_update(&nav,true,5100,30,700,500));
 assert(!r1_nav_button_update(&nav,true,5130,30,700,500));
 assert(!r1_nav_button_update(&nav,true,5829,30,700,500));
 assert(r1_nav_button_update(&nav,true,5830,30,700,500)==R1_NAV_UP);
 /* Configurable 1000ms period and repeated polls at the same time. */
 assert(!r1_nav_button_update(&nav,true,6829,30,700,1000));
 assert(r1_nav_button_update(&nav,true,6830,30,700,1000)==R1_NAV_UP);
 assert(!r1_nav_button_update(&nav,true,6830,30,700,1000));
 /* Reset on connection loss while held suppresses all ups until release. */
 r1_button_init(&nav,true,7000);
 assert(!r1_nav_button_update(&nav,true,10000,30,700,500));
 assert(!r1_nav_button_update(&nav,false,10010,30,700,500));
 assert(!r1_nav_button_update(&nav,false,10040,30,700,500));
 /* Repeat period and first hold cross the uint32 clock boundary. */
 r1_button_init(&nav,false,UINT32_MAX-820);
 assert(!r1_nav_button_update(&nav,true,UINT32_MAX-800,30,700,500));
 assert(!r1_nav_button_update(&nav,true,UINT32_MAX-770,30,700,500));
 assert(r1_nav_button_update(&nav,true,UINT32_MAX-70,30,700,500)==R1_NAV_UP);
 assert(!r1_nav_button_update(&nav,true,428,30,700,500));
 assert(r1_nav_button_update(&nav,true,429,30,700,500)==R1_NAV_UP);
 puts("input tests passed: tap/navigation regressions, 500ms repeat, configurable period, release/bounce stop, no catch-up, connection reset, timer wrap");
}
