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

 /* Tap then hold resolves the pending tap before HOLD, once each. */
 r1_button_init(&b,false,0);
 assert(!r1_button_update(&b,true,10,30,700,300,200));
 assert(!r1_button_update(&b,true,40,30,700,300,200));
 assert(!r1_button_update(&b,false,100,30,700,300,200));
 assert(!r1_button_update(&b,false,130,30,700,300,200));
 assert(!r1_button_update(&b,true,200,30,700,300,200));
 assert(!r1_button_update(&b,true,230,30,700,300,200));
 assert(!r1_button_update(&b,true,429,30,700,300,200));
 assert(r1_button_update(&b,true,430,30,700,300,200)==(R1_INPUT_CLICK|R1_INPUT_HOLD));
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
 assert(r1_button_update(&b,true,660,30,700,300,200)==(R1_INPUT_CLICK|R1_INPUT_HOLD));

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

 /* Late release poll preserves tap, hold and release. */
 r1_button_init(&b,false,0);
 assert(!r1_button_update(&b,true,10,30,700,300,200));
 assert(!r1_button_update(&b,true,40,30,700,300,200));
 assert(!r1_button_update(&b,false,100,30,700,300,200));
 assert(!r1_button_update(&b,false,130,30,700,300,200));
 assert(!r1_button_update(&b,true,200,30,700,300,200));
 assert(!r1_button_update(&b,true,230,30,700,300,200));
 assert(!r1_button_update(&b,false,500,30,700,300,200));
 assert(r1_button_update(&b,false,530,30,700,300,200)==(R1_INPUT_CLICK|R1_INPUT_HOLD|R1_INPUT_RELEASE));

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

 puts("input tests passed: quadrature, debounce, standalone hold/release, delayed single, double, tap-then-hold, boundaries, boot-held, timer wrap, late poll");
}
