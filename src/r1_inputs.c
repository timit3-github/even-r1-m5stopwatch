#include "r1_inputs.h"

void r1_encoder_init(struct r1_encoder *s,uint8_t phase) {
 *s=(struct r1_encoder){.phase=phase&3};
}
int r1_encoder_update(struct r1_encoder *s,uint8_t phase,unsigned edges) {
 static const int8_t delta[16]={0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
 phase&=3;
 if(!edges || edges>4) return 0;
 if((s->phase^phase)==3) {s->partial=0;s->phase=phase;return 0;}
 s->partial+=delta[(s->phase<<2)|phase];s->phase=phase;
 if(s->partial>=(int)edges) {s->partial=0;return 1;}
 if(s->partial<=-(int)edges) {s->partial=0;return -1;}
 return 0;
}
void r1_button_init(struct r1_button *s,bool down,uint32_t now) {
 *s=(struct r1_button){.raw=down,.stable=down,.armed=!down,.changed_ms=now};
}
unsigned r1_button_update(struct r1_button *s,bool down,uint32_t now,
                         uint32_t debounce_ms,uint32_t hold_ms,
                         uint32_t double_ms,uint32_t followup_hold_ms) {
 unsigned events=0;
 if(down!=s->raw) {s->raw=down;s->changed_ms=now;}
 if(s->stable!=s->raw && (uint32_t)(now-s->changed_ms)>=debounce_ms) {
  s->stable=s->raw;
  if(s->stable) {
   s->pressed_ms=now;s->held=false;
   s->followup=s->pending_click && (uint32_t)(s->changed_ms-s->tap_ms)<=double_ms;
   if(s->pending_click && !s->followup) {events|=R1_INPUT_CLICK;s->pending_click=false;}
   s->active_hold_ms=s->followup?followup_hold_ms:hold_ms;
  }
  else if(!s->armed) s->armed=true; /* Ignore a button held at boot. */
  else if(s->held) events=R1_INPUT_RELEASE;
  else if((uint32_t)(s->changed_ms-s->pressed_ms)>=s->active_hold_ms) {
   if(s->followup) {events|=R1_INPUT_TAP_HOLD;s->pending_click=false;}
   else events|=R1_INPUT_HOLD;
   events|=R1_INPUT_RELEASE;
  }
  else if(s->followup) {events|=R1_INPUT_DOUBLE;s->pending_click=false;}
  else {s->pending_click=true;s->tap_ms=now;}
 }
 if(s->armed && s->stable && s->raw && !s->held &&
    (uint32_t)(now-s->pressed_ms)>=s->active_hold_ms) {
  if(s->followup) {events|=R1_INPUT_TAP_HOLD;s->pending_click=false;}
  else events|=R1_INPUT_HOLD;
  s->held=true;
 }
 /* Keep a candidate second press through debounce, including at the boundary. */
 if(s->pending_click && !s->stable && !s->raw &&
    (uint32_t)(now-s->tap_ms)>=double_ms) {
  s->pending_click=false;events|=R1_INPUT_CLICK;
 }
 return events;
}
unsigned r1_nav_button_update(struct r1_button *s,bool down,uint32_t now,
                             uint32_t debounce_ms,uint32_t hold_ms,uint32_t repeat_ms) {
 unsigned events=r1_button_update(s,down,now,debounce_ms,hold_ms,0,hold_ms);
 if(events&R1_INPUT_HOLD) {s->nav_repeat_ms=now;return R1_NAV_UP;}
 if(events&R1_INPUT_CLICK) return R1_NAV_DOWN;
 if(repeat_ms && s->armed && s->held && s->stable && s->raw &&
    (uint32_t)(now-s->changed_ms)>=debounce_ms &&
    (uint32_t)(now-s->nav_repeat_ms)>=repeat_ms) {
  s->nav_repeat_ms=now;return R1_NAV_UP;
 }
 return R1_NAV_NONE;
}
