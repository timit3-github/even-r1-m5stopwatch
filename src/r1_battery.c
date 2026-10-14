/* SPDX-License-Identifier: BSL-1.0 AND MIT
 * Copyright (c) 2026 even-r1-esp32s3 contributors.
 * Copyright (c) 2025 M5Stack Technology CO LTD. See NOTICE.md and LICENSES. */
#include "r1_battery.h"
#include "r1_config.h"
#include "r1_power.h"
#include "r1_usb_power.h"
#include "esp_pm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#if R1_BATTERY_PM1_ENABLED
#include "r1_i2c.h"
#endif
static portMUX_TYPE battery_lock=portMUX_INITIALIZER_UNLOCKED;
static struct r1_battery battery={.percent=R1_BATTERY_PERCENT};
static struct r1_usb_power usb_power;
struct r1_usb_power r1_usb_power_get(void) {
 portENTER_CRITICAL(&battery_lock);
 struct r1_usb_power copy=usb_power;
 portEXIT_CRITICAL(&battery_lock);
 return copy;
}
struct r1_battery r1_battery_get(void) {
 portENTER_CRITICAL(&battery_lock);
 struct r1_battery copy=battery;
 portEXIT_CRITICAL(&battery_lock);
 return copy;
}
#if R1_BATTERY_PM1_ENABLED
_Static_assert(R1_BATTERY_POLL_MS>=100,"battery polling period too short");
_Static_assert(R1_IDLE_BATTERY_POLL_MS>=R1_BATTERY_POLL_MS,"standby battery period must not be shorter than active period");
_Static_assert(R1_BATTERY_FULL_MV>R1_BATTERY_EMPTY_MV,"invalid battery voltage range");
_Static_assert(R1_USB_VIN_POLL_MS>=100,"VIN polling period too short");
_Static_assert(R1_USB_VIN_OFF_MV<R1_USB_VIN_ON_MV && R1_USB_VIN_ON_MV<R1_USB_VIN_MAX_MV,"invalid VIN thresholds");
_Static_assert(R1_USB_VIN_RELEASE_MS>=R1_USB_VIN_POLL_MS,"VIN release needs repeated readings");
static i2c_master_dev_handle_t pm1;
static esp_pm_lock_handle_t vin_awake_lock;
/* PM1 readVbat/readVin return little-endian millivolts at 0x22/0x24.
 * No power, charging, watchdog, GPIO or I2C configuration registers are written. */
static esp_err_t read_voltage(uint8_t reg,uint16_t *mv) {
 uint8_t bytes[2];
 esp_err_t e=ESP_FAIL;
 for(unsigned i=0;i<3;i++) {
  e=r1_i2c_read(pm1,reg,bytes,2);
  if(e==ESP_OK) {*mv=(uint16_t)bytes[0]|((uint16_t)bytes[1]<<8);return e;}
  vTaskDelay(pdMS_TO_TICKS(10));
 }
 return e;
}
static void sample_vin(void) {
 uint16_t mv=0;
 esp_err_t e=read_voltage(0x24,&mv);
 struct r1_usb_power old=r1_usb_power_get(),s=old;
 bool valid=e==ESP_OK && mv<=R1_USB_VIN_MAX_MV;
 r1_usb_power_update(&s,valid,mv,esp_timer_get_time()/1000,
                     R1_USB_VIN_ON_MV,R1_USB_VIN_OFF_MV,R1_USB_VIN_RELEASE_MS);
 if(s.lock_held!=old.lock_held) {
  if(s.lock_held) ESP_ERROR_CHECK(esp_pm_lock_acquire(vin_awake_lock));
  else ESP_ERROR_CHECK(esp_pm_lock_release(vin_awake_lock));
 }
 portENTER_CRITICAL(&battery_lock);usb_power=s;portEXIT_CRITICAL(&battery_lock);
 /* Log state changes and occasional failures, not every voltage reading. */
 if((valid && (!old.valid || old.present!=s.present ||
              old.lock_held!=s.lock_held || old.release_pending!=s.release_pending)) ||
    (!valid && (!s.failures || s.failures%30==1)))
  ESP_LOGI("R1USB","VIN valid=%d mv=%u present=%d lock=%d release_pending=%d error=%s failures=%lu",
           valid,mv,s.present,s.lock_held,s.release_pending,
           esp_err_to_name(e),(unsigned long)s.failures);
}
static esp_err_t set_host_speed(uint32_t speed) {
 if(pm1) {esp_err_t e=r1_i2c_remove(pm1);if(e!=ESP_OK) return e;pm1=NULL;}
 return r1_i2c_add(0x6e,speed,&pm1);
}
static void battery_task(void *unused) {
 (void)unused;
 uint32_t speed=100000;
 esp_err_t e=set_host_speed(speed);
 if(e!=ESP_OK) {ESP_LOGE("R1BAT","PM1_ADD_FAILED %s",esp_err_to_name(e));vTaskDelete(NULL);return;}
 struct r1_battery state={0};
 unsigned failures=0;
 int64_t battery_due_ms=0;
 int64_t vin_due_ms=0;
 for(;;) {
  int64_t current=esp_timer_get_time()/1000;
  bool battery_due=current>=battery_due_ms;
  bool wake_pm1=battery_due;
  bool vin_due=current>=vin_due_ms;
  wake_pm1|=vin_due;
  if(wake_pm1) {
   (void)r1_i2c_probe(0x6e); /* START/address wakes sleeping PM1 */
   vTaskDelay(pdMS_TO_TICKS(10));
  }
  if(vin_due) {
   sample_vin();
   vin_due_ms=esp_timer_get_time()/1000+R1_USB_VIN_POLL_MS;
  }
  if(battery_due) {
   uint16_t mv=0;
   e=read_voltage(0x22,&mv);
   if(e==ESP_OK && r1_battery_update(&state,mv,esp_timer_get_time()/1000,
                                    R1_BATTERY_EMPTY_MV,R1_BATTERY_FULL_MV)) {
    struct r1_battery old=r1_battery_get();
    portENTER_CRITICAL(&battery_lock);battery=state;portEXIT_CRITICAL(&battery_lock);
    if(!old.valid || old.percent!=state.percent || failures)
     ESP_LOGI("R1BAT","BATTERY mv=%u filtered_mv=%u percent=%u i2c_hz=%lu",mv,state.millivolts,state.percent,(unsigned long)speed);
    failures=0;
   } else {
    if(failures++%30==0) ESP_LOGW("R1BAT","BATTERY_READ_FAILED error=%s mv=%u valid=%d; retaining last value (initial fallback=%d)",esp_err_to_name(e),mv,state.valid,R1_BATTERY_PERCENT);
    /* Match official driver's 100K/400K recovery, changing host speed only. */
    speed=speed==100000?400000:100000;
    e=set_host_speed(speed);
    if(e!=ESP_OK) {ESP_LOGE("R1BAT","PM1_READD_FAILED %s",esp_err_to_name(e));vTaskDelete(NULL);return;}
   }
   battery_due_ms=esp_timer_get_time()/1000+
                   (r1_power_waiting()?R1_IDLE_BATTERY_POLL_MS:R1_BATTERY_POLL_MS);
  }
  current=esp_timer_get_time()/1000;
  int64_t wait_ms=battery_due_ms-current;
  if(vin_due_ms-current<wait_ms) wait_ms=vin_due_ms-current;
  vTaskDelay(pdMS_TO_TICKS(wait_ms>0?wait_ms:1));
 }
}
#endif
void r1_battery_init(void) {
#if R1_BATTERY_PM1_ENABLED
 ESP_ERROR_CHECK(esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP,0,"usb_vin",&vin_awake_lock));
 if(xTaskCreate(battery_task,"r1_battery",3072,NULL,1,NULL)!=pdPASS)
  ESP_LOGE("R1BAT","BATTERY_TASK_FAILED; fallback=%d",R1_BATTERY_PERCENT);
#endif
}
