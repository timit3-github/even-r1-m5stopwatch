#pragma once
#include <stdbool.h>
#include <stdint.h>
struct r1_battery {
 bool valid;
 uint8_t percent;
 uint16_t millivolts;
 int64_t sample_ms;
};
void r1_battery_init(void);
struct r1_battery r1_battery_get(void);
/* Pure conversion/filter, also used by the host tests. */
bool r1_battery_update(struct r1_battery *state,uint16_t mv,int64_t now_ms,
                       uint16_t empty_mv,uint16_t full_mv);
