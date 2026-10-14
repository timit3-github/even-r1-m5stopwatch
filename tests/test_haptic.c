/* SPDX-License-Identifier: BSL-1.0
 * Copyright (c) 2026 even-r1-esp32s3 contributors. */
#include <assert.h>
#include <stdarg.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Exercise the production driver with injected bus/startup/resource faults. */
#include "../src/r1_haptic.c"
struct device {uint8_t addr;uint32_t hz;};
struct queue {unsigned values[2],count;};
static uint8_t registers[256];
static unsigned active_locks,live_locks,live_devices,init_calls,pulses_on,pulses_off,waits;
static unsigned mismatch_count,off_failures,on_failures;
static unsigned unavailable_worker,unavailable_full,init_failed,ready_logs;
static unsigned task_fail,queue_fail;
static uint32_t elapsed,chip_ready_ms;
static uint8_t chip_addr=0x4f;
static uint32_t chip_hz=100000;
static TaskFunction_t worker;
static jmp_buf wait_queue;
static char failed_step[32];
const char *esp_err_to_name(esp_err_t e) {return e==ESP_OK?"ESP_OK":e==ESP_FAIL?"ESP_FAIL":"ESP_ERR_TIMEOUT";}
esp_err_t esp_pm_lock_create(int type,int arg,const char *name,esp_pm_lock_handle_t *out) {
 (void)arg;(void)name;assert(type==ESP_PM_NO_LIGHT_SLEEP);*out=malloc(1);assert(*out);live_locks++;return ESP_OK;
}
esp_err_t esp_pm_lock_acquire(esp_pm_lock_handle_t lock) {assert(lock);active_locks++;return ESP_OK;}
esp_err_t esp_pm_lock_release(esp_pm_lock_handle_t lock) {assert(lock && active_locks==1);active_locks--;return ESP_OK;}
esp_err_t esp_pm_lock_delete(esp_pm_lock_handle_t lock) {assert(lock && !active_locks);free(lock);live_locks--;return ESP_OK;}
void test_haptic_log(const char *tag,const char *format,...) {
 (void)tag;char line[256];va_list args;va_start(args,format);vsnprintf(line,sizeof(line),format,args);va_end(args);
 if(strstr(line,"reason=no_worker")) unavailable_worker++;
 if(strstr(line,"reason=queue_full")) unavailable_full++;
 if(strstr(line,"INIT_FAILED")) init_failed++;
 if(strncmp(line,"READY ",6)==0) ready_logs++;
 if(strstr(line,"step=frequency_verify")) strcpy(failed_step,"frequency_verify");
}
QueueHandle_t xQueueCreate(unsigned length,size_t size) {
 assert(length==2 && size==sizeof(unsigned));return queue_fail?NULL:calloc(1,sizeof(struct queue));
}
BaseType_t xQueueSend(QueueHandle_t q,const void *item,TickType_t wait) {
 assert(q && wait==0);struct queue *queue=q;if(queue->count==2) return pdFALSE;
 queue->values[queue->count++]=*(const unsigned *)item;return pdTRUE;
}
BaseType_t xQueueReceive(QueueHandle_t q,void *item,TickType_t wait) {
 assert(q && wait==portMAX_DELAY);struct queue *queue=q;
 if(!queue->count) {assert(!active_locks);waits++;longjmp(wait_queue,1);}
 *(unsigned *)item=queue->values[0];queue->values[0]=queue->values[1];queue->count--;return pdTRUE;
}
void vQueueDelete(QueueHandle_t q) {assert(q);free(q);}
BaseType_t xTaskCreate(TaskFunction_t task,const char *name,unsigned stack,void *arg,unsigned priority,TaskHandle_t *out) {
 (void)name;(void)stack;(void)arg;(void)priority;(void)out;
 if(task_fail) return pdFALSE;
 worker=task;return pdPASS;
}
void vTaskDelay(TickType_t ticks) {assert(active_locks==1);elapsed+=ticks;}
esp_err_t r1_i2c_add(uint8_t addr,uint32_t hz,i2c_master_dev_handle_t *out) {
 assert(active_locks==1);struct device *d=malloc(sizeof(*d));assert(d);*d=(struct device){addr,hz};*out=d;
 live_devices++;init_calls++;return ESP_OK;
}
esp_err_t r1_i2c_remove(i2c_master_dev_handle_t d) {assert(d);free(d);live_devices--;return ESP_OK;}
esp_err_t r1_i2c_probe(uint8_t addr) {(void)addr;assert(active_locks==1);return ESP_OK;}
esp_err_t r1_i2c_read(i2c_master_dev_handle_t dev,uint8_t reg,uint8_t *data,size_t n) {
 assert(dev && active_locks==1);struct device *d=dev;
 if(elapsed<chip_ready_ms || d->addr!=chip_addr || d->hz!=chip_hz) return ESP_ERR_TIMEOUT;
 memcpy(data,registers+reg,n);
 if(reg==0x25 && mismatch_count) {mismatch_count--;data[0]^=1;}
 return ESP_OK;
}
esp_err_t r1_i2c_write(i2c_master_dev_handle_t dev,uint8_t reg,const uint8_t *data,size_t n) {
 assert(dev && active_locks==1);
 if(reg==0x1b) {
  bool on=data[0] || (data[1]&15);
  if(on && on_failures) {on_failures--;return ESP_ERR_TIMEOUT;}
  if(!on && pulses_on && off_failures) {off_failures--;return ESP_ERR_TIMEOUT;}
  if(on) pulses_on++;else pulses_off++;
 }
 memcpy(registers+reg,data,n);return ESP_OK;
}
esp_err_t r1_i2c_update16(i2c_master_dev_handle_t dev,uint8_t reg,uint16_t set,uint16_t clear) {
 assert(dev && active_locks==1);uint16_t value=registers[reg]|((uint16_t)registers[reg+1]<<8);
 value=(value|set)&~clear;registers[reg]=(uint8_t)value;registers[reg+1]=(uint8_t)(value>>8);return ESP_OK;
}
static void start_worker_until_idle(void) {
 assert(worker);if(setjmp(wait_queue)==0) worker(NULL);
}
static void consume_feedback(void) {
 struct queue *q=feedback;assert(q && q->count);
 unsigned count=q->values[0];q->values[0]=q->values[1];q->count--;play_feedback(count);
}
static void cleanup(void) {
 assert(!active_locks);
 if(ioe) {assert(r1_i2c_remove(ioe)==ESP_OK);ioe=NULL;}
 if(feedback) {vQueueDelete(feedback);feedback=NULL;}
 if(awake) {assert(esp_pm_lock_delete(awake)==ESP_OK);awake=NULL;}
 assert(!live_devices && !live_locks);
 motor_ready=false;worker=NULL;elapsed=chip_ready_ms=0;chip_addr=0x4f;chip_hz=100000;
 mismatch_count=off_failures=on_failures=task_fail=queue_fail=0;
 init_calls=pulses_on=pulses_off=waits=unavailable_worker=unavailable_full=init_failed=ready_logs=0;
 memset(registers,0,sizeof(registers));failed_step[0]=0;
}
int main(void) {
 /* IOE not ready at power-on: worker/queue survive and bounded retry recovers. */
 chip_ready_ms=500;
 r1_haptic_init();assert(feedback && worker && !init_calls);
 r1_haptic_touch_toggle(false);start_worker_until_idle();
 assert(motor_ready && ready_logs==1 && elapsed>=500 && pulses_on==1 && !unavailable_worker);
 assert(registers[0x1b]==0 && registers[0x1c]==0x80 && !active_locks);
 unsigned calls=init_calls;uint32_t idle_time=elapsed;
 /* Normal on/off only emits requested feedback, without another initialization. */
 r1_haptic_touch_toggle(true);consume_feedback();assert(pulses_on==3 && init_calls==calls);
 assert(elapsed-idle_time==2*(R1_HAPTIC_PULSE_MS+20)+R1_HAPTIC_GAP_MS);
 cleanup();
 /* Exhausted boot retries: no permanent disable, no periodic idle I2C traffic. */
 chip_ready_ms=100000;r1_haptic_init();start_worker_until_idle();
 assert(!motor_ready && feedback && worker && init_failed==1 && waits==1 && !active_locks);
 calls=init_calls;chip_ready_ms=elapsed;
 r1_haptic_touch_toggle(true);assert(init_calls==calls);consume_feedback();
 assert(motor_ready && ready_logs==1 && pulses_on==2 && !unavailable_worker);
 cleanup();
 /* Read-back mismatch is retried and diagnosed, including fallback address/speed. */
 chip_addr=0x6f;chip_hz=400000;mismatch_count=1;
 registers[0x03]=8;registers[0x05]=8; /* touch-reset must stay high/output */
 r1_haptic_init();start_worker_until_idle();
 assert(motor_ready && bus_hz==400000 && address==0x6f && !pulses_on);
 assert(strcmp(failed_step,"frequency_verify")==0);
 assert((registers[0x03]&8) && (registers[0x05]&8));
 cleanup();
 /* Failed motor-on retries initialization on the next toggle; off is still tried. */
 r1_haptic_init();start_worker_until_idle();on_failures=1;
 r1_haptic_touch_toggle(false);consume_feedback();assert(!motor_ready && !pulses_on && !active_locks);
 r1_haptic_touch_toggle(false);consume_feedback();assert(motor_ready && pulses_on==1);
 /* Off failure is retried with sleep inhibited until the motor is verified off. */
 off_failures=2;r1_haptic_touch_toggle(false);consume_feedback();assert(!off_failures && !active_locks);
 assert(registers[0x1b]==0 && registers[0x1c]==0x80);
 /* Full queue and missing worker have separate diagnostics. */
 r1_haptic_touch_toggle(false);r1_haptic_touch_toggle(true);r1_haptic_touch_toggle(false);
 assert(unavailable_full==1 && !unavailable_worker);cleanup();
 task_fail=1;r1_haptic_init();assert(!feedback && !awake && !live_locks);
 r1_haptic_touch_toggle(false);assert(unavailable_worker==1);cleanup();
 queue_fail=1;r1_haptic_init();assert(!feedback && !awake && !live_locks);cleanup();
 puts("haptic tests passed: delayed boot, retry exhaustion/recovery, read-back failure, address/speed fallback, pulse counts, motor faults, resource cleanup, queue diagnostics");
}
