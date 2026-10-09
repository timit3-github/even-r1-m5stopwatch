/* SPDX-License-Identifier: BSL-1.0 AND MIT
 * Copyright (c) 2026 even-r1-esp32s3 contributors.
 * Copyright (c) 2026 M5Stack Technology CO LTD. See NOTICE.md and LICENSES. */
#include "r1_battery.h"
#include <stddef.h>
bool r1_battery_update(struct r1_battery *s,uint16_t mv,int64_t now,
                       uint16_t empty,uint16_t full) {
 /* Reject missing/implausible LiPo readings; preserve the last good sample. */
 if(!s || mv<2500 || mv>4500 || full<=empty) return false;
 uint16_t filtered=s->valid?(uint16_t)(((uint32_t)s->millivolts*7+mv+4)/8):mv;
 uint8_t percent=filtered<=empty?0:filtered>=full?100:
  (uint8_t)(((uint32_t)filtered-empty)*100/(full-empty));
 *s=(struct r1_battery){.valid=true,.percent=percent,.millivolts=filtered,.sample_ms=now};
 return true;
}
