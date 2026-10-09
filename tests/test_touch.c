#include "r1_touch.h"
#include <assert.h>
#include <stdio.h>
static unsigned step(struct r1_touch_gesture *s,bool down,int x,int y,uint32_t ms) {
 return r1_touch_gesture_update(s,down,x,y,ms,700,300,200,20,60);
}
static unsigned lift(struct r1_touch_gesture *s,uint32_t ms) {return step(s,false,-1,-1,ms);}
int main(void) {
 struct r1_touch_sample sample;
 uint8_t frame[]={0,0,1,0x81,0x2c,0x01,0x90}; /* contact x300 y400 */
 assert(r1_touch_decode(frame,7,&sample));
 assert(sample.down && sample.position && sample.x==300 && sample.y==400);
 frame[3]=0x41;assert(r1_touch_decode(frame,7,&sample));assert(!sample.down && sample.position);
 frame[2]=0;assert(r1_touch_decode(frame,7,&sample));assert(!sample.down && !sample.position);
 frame[2]=1;frame[3]=0xc1;assert(!r1_touch_decode(frame,7,&sample));
 frame[3]=0x81;frame[4]=0xff;assert(!r1_touch_decode(frame,7,&sample));
 assert(!r1_touch_decode(frame,6,&sample));
 struct r1_touch_gesture s;
 r1_touch_gesture_init(&s,false,0);
 assert(!step(&s,true,200,200,100));assert(!lift(&s,180));
 assert(lift(&s,480)==R1_INPUT_CLICK); /* delayed single tap */
 assert(!step(&s,true,40,40,600));assert(!lift(&s,680));
 assert(!step(&s,true,400,400,760));
 assert(lift(&s,840)==R1_INPUT_DOUBLE); /* tap coordinates do not need to match */
 assert(!step(&s,true,200,200,1000));
 assert(!step(&s,true,210,210,1699));
 assert(step(&s,true,210,210,1700)==R1_INPUT_HOLD);
 assert(!step(&s,true,200,100,1750)); /* hold already classified: no swipe */
 assert(lift(&s,1800)==R1_INPUT_RELEASE);
 assert(!step(&s,true,100,100,2000));assert(!lift(&s,2080));
 assert(!step(&s,true,400,400,2150));
 assert(step(&s,true,400,400,2350)==R1_INPUT_TAP_HOLD);
 assert(lift(&s,2400)==R1_INPUT_RELEASE);
 assert(!step(&s,true,200,300,3000));
 assert(!step(&s,true,200,241,3040));
 assert(step(&s,true,200,240,3060)==R1_INPUT_UP); /* exact threshold */
 assert(!step(&s,true,200,100,3100));assert(!lift(&s,3120));assert(!lift(&s,3500));
 assert(!step(&s,true,200,100,4000));
 assert(step(&s,true,200,160,4040)==R1_INPUT_DOWN);assert(!lift(&s,4100));
 /* Horizontal movement cancels tap/hold but produces no vertical swipe. */
 assert(!step(&s,true,100,100,5000));assert(!step(&s,true,200,160,5040));
 assert(!step(&s,true,210,170,6000));assert(!lift(&s,6100));assert(!lift(&s,6500));
 /* A prior real tap is retained if the next contact becomes a swipe. */
 assert(!step(&s,true,200,200,7000));assert(!lift(&s,7080));
 assert(!step(&s,true,200,200,7150));
 assert(step(&s,true,200,260,7180)==(R1_INPUT_CLICK|R1_INPUT_DOWN));assert(!lift(&s,7200));
 /* Explicit UP event can carry the last coordinates of a fast swipe. */
 assert(!step(&s,true,200,200,8000));
 assert(step(&s,false,200,140,8020)==R1_INPUT_UP);assert(!lift(&s,8400));
 /* Reset/boot/reconnect while touching: ignore until a real release. */
 r1_touch_gesture_init(&s,true,9000);
 assert(!step(&s,true,200,200,10000));assert(!step(&s,true,200,100,10100));assert(!lift(&s,10200));
 assert(!step(&s,true,200,200,10300));assert(!lift(&s,10400));assert(lift(&s,10700)==R1_INPUT_CLICK);
 /* Millisecond timer wrap still supports hold. */
 r1_touch_gesture_init(&s,false,UINT32_MAX-200);
 assert(!step(&s,true,200,200,UINT32_MAX-100));assert(step(&s,true,200,200,599)==R1_INPUT_HOLD);
 puts("touch tests passed: frame decode, position-independent taps, hold/menu, swipe boundaries, cancellation, reset, timer wrap");
}
