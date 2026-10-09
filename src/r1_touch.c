/* SPDX-License-Identifier: BSL-1.0 AND MIT
 * Copyright (c) 2026 even-r1-esp32s3 contributors.
 * Copyright (c) 2026 M5Stack Technology CO LTD. See NOTICE.md and LICENSES. */
#include "r1_touch.h"
#include "r1_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#if R1_TOUCH_ENABLED
#include "r1_i2c.h"
#endif
static QueueHandle_t samples;
static volatile unsigned overflow;
static portMUX_TYPE sample_lock=portMUX_INITIALIZER_UNLOCKED;
static struct r1_touch_sample latest;
struct r1_touch_sample r1_touch_hw_get(void) {
 portENTER_CRITICAL(&sample_lock);struct r1_touch_sample s=latest;portEXIT_CRITICAL(&sample_lock);return s;
}
bool r1_touch_hw_pop(struct r1_touch_sample *s) {return samples && xQueueReceive(samples,s,0)==pdTRUE;}
bool r1_touch_hw_overflow(void) {
 bool result=__atomic_exchange_n(&overflow,0,__ATOMIC_RELAXED)!=0;
 if(result && samples) xQueueReset(samples);
 return result;
}
#if R1_TOUCH_ENABLED
_Static_assert(R1_TOUCH_POLL_MS>=10,"touch poll period too short");
_Static_assert(R1_TOUCH_SWIPE_PX>R1_TOUCH_TAP_SLOP_PX,"swipe threshold must exceed tap slop");
static i2c_master_dev_handle_t ioe,cst;
/* M5IOE1_PIN_4 is enum value 3 (bit3). Touch is powered by L2.
 * Update only touch-reset, preserving display and other peripherals. */
static esp_err_t update_reg16(uint8_t reg,uint16_t set,uint16_t clear) {
 uint8_t b[2];esp_err_t e=r1_i2c_read(ioe,reg,b,2);if(e!=ESP_OK) return e;
 uint16_t value=((uint16_t)b[0]|((uint16_t)b[1]<<8));value=(value|set)&~clear;
 b[0]=(uint8_t)value;b[1]=(uint8_t)(value>>8);
 e=r1_i2c_write(ioe,reg,b,2);if(e!=ESP_OK) return e;
 vTaskDelay(pdMS_TO_TICKS(1));
 e=r1_i2c_read(ioe,reg,b,2);
 if(e==ESP_OK && (((uint16_t)b[0]|((uint16_t)b[1]<<8))&(set|clear))!=(value&(set|clear))) return ESP_FAIL;
 return e;
}
static esp_err_t reset_touch(void) {
 const uint16_t bits=1u<<3;
 (void)r1_i2c_probe(0x4f);(void)r1_i2c_probe(0x6f);vTaskDelay(pdMS_TO_TICKS(10));
 /* Match official pinMode OUTPUT: remove pulls, push-pull, output direction. */
 esp_err_t e=update_reg16(0x09,0,bits);if(e!=ESP_OK) return e;
 e=update_reg16(0x0b,0,bits);if(e!=ESP_OK) return e;
 e=update_reg16(0x13,0,bits);if(e!=ESP_OK) return e;
 e=update_reg16(0x05,0,bits);if(e!=ESP_OK) return e;
 e=update_reg16(0x03,bits,0);if(e!=ESP_OK) return e;
 vTaskDelay(pdMS_TO_TICKS(10));
 e=update_reg16(0x05,bits,0);if(e!=ESP_OK) return e;
 vTaskDelay(pdMS_TO_TICKS(50));return ESP_OK;
}
static esp_err_t attach_ioe(void) {
 const uint8_t addresses[]={0x4f,0x6f};
 for(unsigned a=0;a<2;a++) for(unsigned f=0;f<2;f++) {
  if(ioe) {esp_err_t e=r1_i2c_remove(ioe);if(e!=ESP_OK) return e;ioe=NULL;}
  esp_err_t e=r1_i2c_add(addresses[a],f?400000:100000,&ioe);if(e!=ESP_OK) return e;
  (void)r1_i2c_probe(addresses[a]);vTaskDelay(pdMS_TO_TICKS(10));
  uint8_t rev;
  if(r1_i2c_read(ioe,0x02,&rev,1)==ESP_OK && reset_touch()==ESP_OK) {
   ESP_LOGI("R1TP","IOE_READY addr=%02x hz=%u rev=%u",addresses[a],f?400000:100000,rev);return ESP_OK;
  }
 }
 return ESP_FAIL;
}
static void publish(struct r1_touch_sample *s) {
 s->ms=esp_timer_get_time()/1000;
 portENTER_CRITICAL(&sample_lock);latest=*s;portEXIT_CRITICAL(&sample_lock);
 if(xQueueSend(samples,s,0)!=pdTRUE) __atomic_store_n(&overflow,1,__ATOMIC_RELAXED);
}
static void touch_task(void *unused) {
 (void)unused;
 esp_err_t e=attach_ioe();
 if(e!=ESP_OK) {vTaskDelay(pdMS_TO_TICKS(800));e=attach_ioe();}
 if(e==ESP_OK) e=r1_i2c_add(0x15,100000,&cst);
 if(e!=ESP_OK) {ESP_LOGE("R1TP","TOUCH_INIT_FAILED %s",esp_err_to_name(e));vTaskDelete(NULL);return;}
 uint8_t id=0,version=0;
 e=r1_i2c_read(cst,0xa7,&id,1);
 if(e==ESP_OK) e=r1_i2c_read(cst,0xa9,&version,1);
 ESP_LOGI("R1TP","TOUCH_IDENT error=%s id=%02x fw=%02x",esp_err_to_name(e),id,version);
 unsigned failures=0;
 for(;;) {
  struct r1_touch_sample sample={0};uint8_t bytes[7];
  e=r1_i2c_read(cst,0,bytes,sizeof(bytes));
  if(e==ESP_OK && r1_touch_decode(bytes,sizeof(bytes),&sample)) {
   if(failures) ESP_LOGI("R1TP","TOUCH_READ_RECOVERED");
   failures=0;
  } else {
   if(failures++%250==0) ESP_LOGW("R1TP","TOUCH_READ_FAILED %s",esp_err_to_name(e));
   /* Never turn a failed read into a release/tap/long press. */
   if(failures%250==0) (void)reset_touch();
  }
  publish(&sample);vTaskDelay(pdMS_TO_TICKS(R1_TOUCH_POLL_MS));
 }
}
#endif
void r1_touch_hw_init(void) {
#if R1_TOUCH_ENABLED
 samples=xQueueCreate(32,sizeof(struct r1_touch_sample));
 if(!samples || xTaskCreate(touch_task,"r1_touch",4096,NULL,1,NULL)!=pdPASS)
  ESP_LOGE("R1TP","TOUCH_TASK_FAILED");
#endif
}
