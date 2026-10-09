#include "r1_battery.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
 struct r1_battery b={.percent=100};
 assert(!r1_battery_update(&b,0,1000,3300,4200));
 assert(!b.valid && b.percent==100);
 assert(r1_battery_update(&b,3750,2000,3300,4200));
 assert(b.valid && b.percent==50 && b.millivolts==3750);
 assert(r1_battery_update(&b,3830,3000,3300,4200));
 assert(b.millivolts==3760 && b.percent==51 && b.sample_ms==3000);
 assert(!r1_battery_update(&b,65535,4000,3300,4200));
 assert(b.millivolts==3760 && b.sample_ms==3000);
 assert(!r1_battery_update(&b,3700,4000,4200,3300));
 assert(!r1_battery_update(NULL,3700,4000,3300,4200));
 const uint16_t mv[]={2500,3300,3309,4200,4500};
 const uint8_t expected[]={0,0,1,100,100};
 for(unsigned i=0;i<5;i++) {
  b=(struct r1_battery){0};
  assert(r1_battery_update(&b,mv[i],5000,3300,4200));
  assert(b.percent==expected[i]);
 }
 puts("battery tests passed");
}
