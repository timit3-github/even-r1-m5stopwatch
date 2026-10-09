#include "r1_battery.h"
#include "r1_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#if R1_BATTERY_PM1_ENABLED
#include "driver/i2c_master.h"
#endif
static portMUX_TYPE battery_lock=portMUX_INITIALIZER_UNLOCKED;
static struct r1_battery battery={.percent=R1_BATTERY_PERCENT};
struct r1_battery r1_battery_get(void) {
 portENTER_CRITICAL(&battery_lock);
 struct r1_battery copy=battery;
 portEXIT_CRITICAL(&battery_lock);
 return copy;
}
#if R1_BATTERY_PM1_ENABLED
_Static_assert(R1_BATTERY_POLL_MS>=100,"battery polling period too short");
_Static_assert(R1_BATTERY_FULL_MV>R1_BATTERY_EMPTY_MV,"invalid battery voltage range");
static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t pm1;
/* PM1 readVbat returns little-endian millivolts at register 0x22.
 * No power, charging, watchdog, GPIO or I2C configuration registers are written. */
static esp_err_t read_voltage(uint16_t *mv) {
 (void)i2c_master_probe(bus,0x6e,50); /* START/address wakes sleeping PM1 */
 vTaskDelay(pdMS_TO_TICKS(10));
 const uint8_t reg=0x22;
 uint8_t bytes[2];
 esp_err_t e=ESP_FAIL;
 for(unsigned i=0;i<3;i++) {
  e=i2c_master_transmit_receive(pm1,&reg,1,bytes,2,100);
  if(e==ESP_OK) {*mv=(uint16_t)bytes[0]|((uint16_t)bytes[1]<<8);return e;}
  vTaskDelay(pdMS_TO_TICKS(10));
 }
 return e;
}
static esp_err_t set_host_speed(uint32_t speed) {
 if(pm1) {esp_err_t e=i2c_master_bus_rm_device(pm1);if(e!=ESP_OK) return e;pm1=NULL;}
 i2c_device_config_t cfg={.dev_addr_length=I2C_ADDR_BIT_LEN_7,
  .device_address=0x6e,.scl_speed_hz=speed};
 return i2c_master_bus_add_device(bus,&cfg,&pm1);
}
static void battery_task(void *unused) {
 (void)unused;
 i2c_master_bus_config_t cfg={.i2c_port=I2C_NUM_0,
  .sda_io_num=R1_BATTERY_SDA_GPIO,.scl_io_num=R1_BATTERY_SCL_GPIO,
  .clk_source=I2C_CLK_SRC_DEFAULT,.glitch_ignore_cnt=7,.flags.enable_internal_pullup=true};
 esp_err_t e=i2c_new_master_bus(&cfg,&bus);
 if(e!=ESP_OK) {ESP_LOGE("R1BAT","I2C_INIT_FAILED %s; fallback=%d",esp_err_to_name(e),R1_BATTERY_PERCENT);vTaskDelete(NULL);return;}
 uint32_t speed=100000;
 e=set_host_speed(speed);
 if(e!=ESP_OK) {ESP_LOGE("R1BAT","PM1_ADD_FAILED %s",esp_err_to_name(e));i2c_del_master_bus(bus);vTaskDelete(NULL);return;}
 struct r1_battery state={0};
 unsigned failures=0;
 for(;;) {
  uint16_t mv=0;
  e=read_voltage(&mv);
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
   if(e!=ESP_OK) {ESP_LOGE("R1BAT","PM1_READD_FAILED %s",esp_err_to_name(e));i2c_del_master_bus(bus);vTaskDelete(NULL);return;}
  }
  vTaskDelay(pdMS_TO_TICKS(R1_BATTERY_POLL_MS));
 }
}
#endif
void r1_battery_init(void) {
#if R1_BATTERY_PM1_ENABLED
 if(xTaskCreate(battery_task,"r1_battery",3072,NULL,1,NULL)!=pdPASS)
  ESP_LOGE("R1BAT","BATTERY_TASK_FAILED; fallback=%d",R1_BATTERY_PERCENT);
#endif
}
