/* SPDX-License-Identifier: BSL-1.0
 * Copyright (c) 2026 even-r1-esp32s3 contributors. */
#include "r1_inputs.h"
#include "r1_input_queue.h"
#include <assert.h>
#include <stdio.h>
static unsigned chord(struct r1_chord *s,unsigned raw,uint32_t ms) {
 return r1_chord_update(s,raw,ms,30,100,500);
}
int main(void) {
 struct r1_chord s;r1_chord_init(&s,0,0);
 assert(!chord(&s,1,100));assert(chord(&s,3,200)==(R1_CHORD_START|R1_CHORD_BLOCK));
 assert(chord(&s,3,699)==R1_CHORD_BLOCK);
 assert(chord(&s,3,700)==(R1_CHORD_TOGGLE|R1_CHORD_BLOCK));
 assert(chord(&s,3,5000)==R1_CHORD_BLOCK); /* one toggle per hold */
 assert(chord(&s,1,5010)==R1_CHORD_BLOCK);assert(chord(&s,3,5100)==R1_CHORD_BLOCK);
 assert(chord(&s,0,5200)==R1_CHORD_BLOCK);assert(chord(&s,0,5229)==R1_CHORD_BLOCK);
 assert(chord(&s,0,5230)==R1_CHORD_BLOCK);assert(!chord(&s,0,5231));
 /* Opposite order, aborted chord, and too wide a separation. */
 assert(!chord(&s,2,6000));assert(chord(&s,3,6050)&R1_CHORD_START);
 assert(chord(&s,2,6300)==R1_CHORD_BLOCK);assert(chord(&s,3,6500)==R1_CHORD_BLOCK);
 assert(chord(&s,3,8000)==R1_CHORD_BLOCK);assert(chord(&s,0,8010)==R1_CHORD_BLOCK);
 assert(chord(&s,0,8040)==R1_CHORD_BLOCK);
 assert(!chord(&s,1,9000));assert(!chord(&s,3,9101));assert(!chord(&s,3,10000));
 /* Buttons held at startup are ignored until both are released. */
 r1_chord_init(&s,3,0);assert(chord(&s,3,1000)==R1_CHORD_BLOCK);
 assert(chord(&s,0,1100)==R1_CHORD_BLOCK);assert(chord(&s,0,1130)==R1_CHORD_BLOCK);
 assert(chord(&s,3,1200)&R1_CHORD_START);assert(chord(&s,3,1700)&R1_CHORD_TOGGLE);
 /* Contact bounce invalidates a candidate rather than producing a toggle. */
 r1_chord_init(&s,0,0);assert(chord(&s,3,10)&R1_CHORD_START);
 assert(chord(&s,1,20)==R1_CHORD_BLOCK);assert(chord(&s,3,25)==R1_CHORD_BLOCK);
 assert(chord(&s,3,1000)==R1_CHORD_BLOCK);
 r1_chord_init(&s,0,UINT32_MAX-200);
 assert(!chord(&s,2,UINT32_MAX-100));assert(chord(&s,3,UINT32_MAX-50)&R1_CHORD_START);
 assert(chord(&s,3,448)==R1_CHORD_BLOCK);assert(chord(&s,3,449)&R1_CHORD_TOGGLE);
 /* Wrapped pacing queue: retain normal inputs, cancel every pending repeat. */
 struct r1_input_queue q={0};
 for(unsigned i=0;i<7;i++) {
  assert(r1_input_queue_push(&q,(struct r1_pending_input){'s',R1_SOURCE_G1,false}));
  r1_input_queue_pop(&q);
 }
 assert(r1_input_queue_push(&q,(struct r1_pending_input){'s',R1_SOURCE_G1,false}));
 assert(r1_input_queue_push(&q,(struct r1_pending_input){'j',R1_SOURCE_NAV,false}));
 assert(r1_input_queue_push(&q,(struct r1_pending_input){'j',R1_SOURCE_NAV,true}));
 assert(r1_input_queue_push(&q,(struct r1_pending_input){'j',R1_SOURCE_NAV,true}));
 assert(q.count==3);
 assert(r1_input_queue_push(&q,(struct r1_pending_input){'u',R1_SOURCE_TOUCH,true}));
 r1_input_queue_cancel(&q,R1_SOURCE_NAV,true);assert(q.count==3);
 assert(r1_input_queue_peek(&q)->command=='s');r1_input_queue_pop(&q);
 assert(!r1_input_queue_peek(&q)->repeat);r1_input_queue_pop(&q);
 assert(r1_input_queue_peek(&q)->source==R1_SOURCE_TOUCH);
 r1_input_queue_cancel(&q,R1_SOURCE_TOUCH,false);assert(!r1_input_queue_peek(&q));
 for(unsigned i=0;i<8;i++) assert(r1_input_queue_push(&q,(struct r1_pending_input){'d',R1_SOURCE_G1,false}));
 assert(!r1_input_queue_push(&q,(struct r1_pending_input){'h',R1_SOURCE_G1,false}));
 r1_input_queue_cancel(&q,R1_SOURCE_ALL,false);assert(q.count==0);
 puts("chord tests passed: priority, abort/bounce, boot hold, debounce, wrap, repeat coalescing/cancellation, wrapped queue");
}
