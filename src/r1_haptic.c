/* SPDX-License-Identifier: BSL-1.0 AND MIT
 * Copyright (c) 2026 even-r1-esp32s3 contributors.
 * Copyright (c) 2026 M5Stack Technology CO LTD. See NOTICE.md and LICENSES. */
#include "r1_haptic.h"
#include "r1_config.h"
#if R1_HAPTIC_ENABLED
#include "r1_i2c.h"
#include "esp_pm.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
static i2c_master_dev_handle_t ioe;
static uint8_t address;
static uint32_t bus_hz;
static QueueHandle_t feedback;
static esp_pm_lock_handle_t awake;
static bool motor_ready;
static const char *init_step="attach";
enum {INIT_ATTEMPTS=3,INIT_RETRY_MS=800};
_Static_assert(R1_HAPTIC_DUTY_PERCENT>0 && R1_HAPTIC_DUTY_PERCENT<=100,"invalid motor duty");
_Static_assert(R1_HAPTIC_PULSE_MS>0 && R1_HAPTIC_GAP_MS>0,"invalid feedback timing");
/* IO9 (bit8), PWM channel0. Duty is 12bit plus enable bit15. */
static esp_err_t motor(unsigned percent) {
 (void)r1_i2c_probe(address);vTaskDelay(pdMS_TO_TICKS(10));
 uint16_t value=(uint16_t)(percent*4095/100)|0x8000;
 uint8_t b[2]={(uint8_t)value,(uint8_t)(value>>8)},check[2];
 esp_err_t e=r1_i2c_write(ioe,0x1b,b,2);
 if(e==ESP_OK) e=r1_i2c_read(ioe,0x1b,check,2);
 if(e==ESP_OK && (check[0]!=b[0] || check[1]!=b[1])) e=ESP_FAIL;
 return e;
}
static void stop_motor(void) {
 /* If a bus error follows motor-on, keep retrying off before going idle. */
 unsigned failures=0;
 for(;;) {
  esp_err_t e=motor(0);
  if(e==ESP_OK) return;
  if(failures++%50==0) ESP_LOGE("R1HAPTIC","MOTOR_OFF_FAILED %s",esp_err_to_name(e));
  vTaskDelay(pdMS_TO_TICKS(100));
 }
}
static esp_err_t initialize_motor_once(void) {
 init_step="attach";
 const uint8_t addresses[]={0x4f,0x6f};esp_err_t e=ESP_FAIL;
 /* A failed attempt must not leave an old device handle behind. */
 if(ioe) {
  e=r1_i2c_remove(ioe);if(e!=ESP_OK) return e;
  ioe=NULL;
 }
 for(unsigned a=0;a<2;a++) for(unsigned f=0;f<2;f++) {
  address=addresses[a];bus_hz=f?400000:100000;
  e=r1_i2c_add(address,bus_hz,&ioe);
  if(e!=ESP_OK) return e;
  (void)r1_i2c_probe(address);vTaskDelay(pdMS_TO_TICKS(10));
  uint8_t rev;
  e=r1_i2c_read(ioe,0x02,&rev,1);
  if(e==ESP_OK) goto attached;
  esp_err_t removed=r1_i2c_remove(ioe);
  if(removed!=ESP_OK) return removed;
  ioe=NULL;
 }
 return e;
attached:
 init_step="motor_off";e=motor(0); /* Off before setting output direction. */
 const uint16_t pin=1u<<8;
 init_step=e==ESP_OK?"pull_up":init_step;
 if(e==ESP_OK) e=r1_i2c_update16(ioe,0x09,0,pin);
 init_step=e==ESP_OK?"pull_down":init_step;
 if(e==ESP_OK) e=r1_i2c_update16(ioe,0x0b,0,pin);
 init_step=e==ESP_OK?"drive":init_step;
 if(e==ESP_OK) e=r1_i2c_update16(ioe,0x13,0,pin);
 init_step=e==ESP_OK?"output_low":init_step;
 if(e==ESP_OK) e=r1_i2c_update16(ioe,0x05,0,pin);
 init_step=e==ESP_OK?"output_mode":init_step;
 if(e==ESP_OK) e=r1_i2c_update16(ioe,0x03,pin,0);
 uint8_t frequency[2]={0x88,0x13}; /* Official StopWatch demo: 5000Hz. */
 init_step=e==ESP_OK?"frequency_write":init_step;
 if(e==ESP_OK) e=r1_i2c_write(ioe,0x25,frequency,2);
 uint8_t actual[2];
 init_step=e==ESP_OK?"frequency_verify":init_step;
 if(e==ESP_OK) e=r1_i2c_read(ioe,0x25,actual,2);
 if(e==ESP_OK && (actual[0]!=frequency[0] || actual[1]!=frequency[1])) e=ESP_FAIL;
 return e;
}
/* Called only by the worker with its NO_LIGHT_SLEEP lock held. */
static bool initialize_motor(void) {
 for(unsigned attempt=0;attempt<INIT_ATTEMPTS;attempt++) {
  if(attempt) vTaskDelay(pdMS_TO_TICKS(INIT_RETRY_MS));
  esp_err_t e=initialize_motor_once();
  if(e==ESP_OK) {
   motor_ready=true;
   ESP_LOGI("R1HAPTIC","READY addr=%02x hz=%lu duty=%u pulse_ms=%u gap_ms=%u",address,
    (unsigned long)bus_hz,R1_HAPTIC_DUTY_PERCENT,R1_HAPTIC_PULSE_MS,R1_HAPTIC_GAP_MS);
   return true;
  }
  ESP_LOGW("R1HAPTIC","INIT_RETRY step=%s attempt=%u/%u error=%s",init_step,
   attempt+1,INIT_ATTEMPTS,esp_err_to_name(e));
 }
 motor_ready=false;
 ESP_LOGE("R1HAPTIC","INIT_FAILED step=%s; retry on next toggle",init_step);
 return false;
}
static void play_feedback(unsigned pulses) {
 ESP_ERROR_CHECK(esp_pm_lock_acquire(awake));
 if(motor_ready || initialize_motor()) {
  for(unsigned i=0;i<pulses;i++) {
   esp_err_t e=motor(R1_HAPTIC_DUTY_PERCENT);
   if(e==ESP_OK) vTaskDelay(pdMS_TO_TICKS(R1_HAPTIC_PULSE_MS));
   else ESP_LOGW("R1HAPTIC","MOTOR_ON_FAILED %s",esp_err_to_name(e));
   stop_motor();
   if(e!=ESP_OK) {motor_ready=false;break;}
   if(i+1<pulses) vTaskDelay(pdMS_TO_TICKS(R1_HAPTIC_GAP_MS));
  }
 }
 ESP_ERROR_CHECK(esp_pm_lock_release(awake));
}
static void haptic_task(void *arg) {
 (void)arg;unsigned pulses;
 ESP_ERROR_CHECK(esp_pm_lock_acquire(awake));
 (void)initialize_motor();
 ESP_ERROR_CHECK(esp_pm_lock_release(awake));
 for(;;) {
  if(xQueueReceive(feedback,&pulses,portMAX_DELAY)==pdTRUE) play_feedback(pulses);
 }
}
#endif
void r1_haptic_init(void) {
#if R1_HAPTIC_ENABLED
 if(feedback) return;
 ESP_ERROR_CHECK(esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP,0,"r1_haptic",&awake));
 feedback=xQueueCreate(2,sizeof(unsigned));
 if(!feedback || xTaskCreate(haptic_task,"r1_haptic",3072,NULL,2,NULL)!=pdPASS) {
  ESP_LOGE("R1HAPTIC","TASK_FAILED");
  if(feedback) vQueueDelete(feedback);
  feedback=NULL;
  ESP_ERROR_CHECK(esp_pm_lock_delete(awake));awake=NULL;return;
 }
 ESP_LOGI("R1HAPTIC","TASK_READY");
#endif
}
void r1_haptic_touch_toggle(bool enabled) {
#if R1_HAPTIC_ENABLED
 unsigned pulses=enabled?2:1;
 if(!feedback) ESP_LOGW("R1HAPTIC","FEEDBACK_UNAVAILABLE reason=no_worker enabled=%d",enabled);
 else if(xQueueSend(feedback,&pulses,0)!=pdTRUE)
  ESP_LOGW("R1HAPTIC","FEEDBACK_UNAVAILABLE reason=queue_full enabled=%d",enabled);
#else
 (void)enabled;
#endif
}
