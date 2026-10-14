/* SPDX-License-Identifier: BSL-1.0
 * Copyright (c) 2026 even-r1-esp32s3 contributors. */
#include "r1_input_queue.h"
#include <stddef.h>
bool r1_input_queue_push(struct r1_input_queue *q,struct r1_pending_input input) {
 if(input.repeat) for(unsigned i=0;i<q->count;i++)
  if(q->items[(q->head+i)%8].repeat && q->items[(q->head+i)%8].source==input.source)
   return true; /* At most one outstanding repeat per source, either direction. */
 if(q->count==8) return false;
 q->items[(q->head+q->count++)%8]=input;return true;
}
const struct r1_pending_input *r1_input_queue_peek(const struct r1_input_queue *q) {
 return q->count?&q->items[q->head]:NULL;
}
void r1_input_queue_pop(struct r1_input_queue *q) {
 if(q->count) {q->head=(q->head+1)%8;q->count--;}
}
void r1_input_queue_cancel(struct r1_input_queue *q,unsigned sources,bool repeats_only) {
 unsigned kept=0;
 for(unsigned i=0;i<q->count;i++) {
  struct r1_pending_input input=q->items[(q->head+i)%8];
  if(!(input.source&sources) || (repeats_only && !input.repeat))
   q->items[(q->head+kept++)%8]=input;
 }
 q->count=kept;
}
