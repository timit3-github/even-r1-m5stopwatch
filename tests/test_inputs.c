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
 assert(!r1_button_update(&b,true,10,30,700));
 assert(!r1_button_update(&b,false,20,30,700));
 assert(!r1_button_update(&b,false,60,30,700)); /* bounce only */
 assert(!r1_button_update(&b,true,100,30,700));
 assert(!r1_button_update(&b,true,129,30,700));
 assert(!r1_button_update(&b,true,130,30,700));
 assert(!r1_button_update(&b,false,200,30,700));
 assert(r1_button_update(&b,false,230,30,700)==R1_INPUT_CLICK);
 assert(!r1_button_update(&b,false,250,30,700));
 assert(!r1_button_update(&b,true,300,30,700));
 assert(!r1_button_update(&b,true,330,30,700));
 assert(!r1_button_update(&b,true,1029,30,700));
 assert(r1_button_update(&b,true,1030,30,700)==R1_INPUT_HOLD);
 assert(!r1_button_update(&b,true,1200,30,700));
 assert(!r1_button_update(&b,false,1300,30,700));
 assert(r1_button_update(&b,false,1330,30,700)==R1_INPUT_RELEASE);
 assert(!r1_button_update(&b,false,1400,30,700));
 r1_button_init(&b,true,0);assert(!r1_button_update(&b,true,1000,30,700));
 assert(!r1_button_update(&b,false,1010,30,700));assert(!r1_button_update(&b,false,1040,30,700));
 /* Timer wrap and late poll spanning the hold threshold. */
 r1_button_init(&b,false,UINT32_MAX-100);
 assert(!r1_button_update(&b,true,UINT32_MAX-80,30,700));
 assert(!r1_button_update(&b,true,UINT32_MAX-50,30,700));
 assert(r1_button_update(&b,true,649,30,700)==R1_INPUT_HOLD);
 assert(!r1_button_update(&b,false,660,30,700));
 assert(r1_button_update(&b,false,690,30,700)==R1_INPUT_RELEASE);
 r1_button_init(&b,false,0);assert(!r1_button_update(&b,true,10,30,700));
 assert(!r1_button_update(&b,true,40,30,700));
 assert(!r1_button_update(&b,false,900,30,700));
 assert(r1_button_update(&b,false,930,30,700)==(R1_INPUT_HOLD|R1_INPUT_RELEASE));
 puts("input tests passed: quadrature direction, detents, bounce, reversal, invalid edge, button debounce/hold/release, boot-held, timer wrap");
}
