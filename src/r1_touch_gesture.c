/* SPDX-License-Identifier: BSL-1.0 AND MIT
 * Copyright (c) 2026 even-r1-esp32s3 contributors.
 * Copyright (c) 2026 M5Stack Technology CO LTD. See NOTICE.md and LICENSES. */
#include "r1_touch.h"
#include <stdlib.h>
bool r1_touch_decode(const uint8_t *p,size_t n,struct r1_touch_sample *s) {
 if(!p || !s || n!=7 || p[2]>1) return false;
 unsigned event=p[3]>>6;
 if(p[2] && event==3) return false;
 bool down=p[2]!=0 && event!=1;
 uint16_t x=((uint16_t)(p[3]&15)<<8)|p[4],y=((uint16_t)(p[5]&15)<<8)|p[6];
 bool position=down || (p[2]!=0 && event==1);
 if(position && (x>=466 || y>=466)) return false;
 *s=(struct r1_touch_sample){.valid=true,.down=down,.position=position,.x=x,.y=y};return true;
}
void r1_touch_gesture_init(struct r1_touch_gesture *s,bool down,uint32_t now) {
 *s=(struct r1_touch_gesture){.contact=down,.moved=down,.swiped=down};
 r1_button_init(&s->button,down,now);
}
unsigned r1_touch_gesture_update(struct r1_touch_gesture *s,bool down,int x,int y,
 uint32_t now,uint32_t hold,uint32_t dbl,uint32_t follow,unsigned slop,unsigned swipe,uint32_t repeat_ms) {
 unsigned events=0;
 if(down && !s->contact) {
  s->start_x=x;s->start_y=y;s->moved=false;s->swiped=false;s->swipe_direction=0;
 }
 bool was_contact=s->contact;
 s->contact=down;
 if((down || was_contact) && x>=0 && y>=0 && !s->button.held) {
  int dx=x-s->start_x,dy=y-s->start_y;
  if(!s->moved && (abs(dx)>(int)slop || abs(dy)>(int)slop)) {
   /* A prior completed tap stays a tap when the next contact is a swipe. */
   if(s->button.pending_click) events|=R1_INPUT_CLICK;
   s->moved=true;r1_button_init(&s->button,true,now);
  }
  if(s->moved && !s->swiped && swipe && abs(dy)>=(int)swipe && abs(dy)>=abs(dx)) {
   s->swiped=true;s->swipe_direction=dy<0?R1_INPUT_UP:R1_INPUT_DOWN;
   s->swipe_ms=now;events|=s->swipe_direction;
  }
 }
 if(s->moved) {
  if(!down) {s->moved=false;s->swipe_direction=0;r1_button_init(&s->button,false,now);}
  else if(s->swipe_direction && repeat_ms && (uint32_t)(now-s->swipe_ms)>=repeat_ms) {
   s->swipe_ms=now;events|=s->swipe_direction|R1_INPUT_REPEAT;
  }
  return events;
 }
 /* CST820 contact state is already sampled; no extra GPIO debounce delay. */
 return events|r1_button_update(&s->button,down,now,0,hold,dbl,follow);
}
