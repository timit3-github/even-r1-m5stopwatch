/* SPDX-License-Identifier: BSL-1.0
 * Copyright (c) 2026 even-r1-esp32s3 contributors. */
#include "r1_i2c.h"
#include "r1_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include <string.h>
static i2c_master_bus_handle_t bus;
static SemaphoreHandle_t mutex;
void r1_i2c_init(void) {
#if R1_BATTERY_PM1_ENABLED || R1_TOUCH_ENABLED
 mutex=xSemaphoreCreateMutex();
 if(!mutex) {ESP_LOGE("R1I2C","MUTEX_FAILED");return;}
 i2c_master_bus_config_t cfg={.i2c_port=I2C_NUM_0,
  .sda_io_num=R1_BATTERY_SDA_GPIO,.scl_io_num=R1_BATTERY_SCL_GPIO,
  .clk_source=I2C_CLK_SRC_DEFAULT,.glitch_ignore_cnt=7,.flags.enable_internal_pullup=true};
 esp_err_t e=i2c_new_master_bus(&cfg,&bus);
 if(e!=ESP_OK) ESP_LOGE("R1I2C","BUS_INIT_FAILED %s",esp_err_to_name(e));
#endif
}
static bool lock(void) {return bus && mutex && xSemaphoreTake(mutex,pdMS_TO_TICKS(50))==pdTRUE;}
esp_err_t r1_i2c_add(uint8_t addr,uint32_t hz,i2c_master_dev_handle_t *dev) {
 if(!lock()) return ESP_ERR_INVALID_STATE;
 i2c_device_config_t cfg={.dev_addr_length=I2C_ADDR_BIT_LEN_7,.device_address=addr,.scl_speed_hz=hz};
 esp_err_t e=i2c_master_bus_add_device(bus,&cfg,dev);xSemaphoreGive(mutex);return e;
}
esp_err_t r1_i2c_remove(i2c_master_dev_handle_t dev) {
 if(!lock()) return ESP_ERR_INVALID_STATE;
 esp_err_t e=i2c_master_bus_rm_device(dev);xSemaphoreGive(mutex);return e;
}
esp_err_t r1_i2c_probe(uint8_t addr) {
 if(!lock()) return ESP_ERR_INVALID_STATE;
 esp_err_t e=i2c_master_probe(bus,addr,20);xSemaphoreGive(mutex);return e;
}
esp_err_t r1_i2c_read(i2c_master_dev_handle_t dev,uint8_t reg,uint8_t *data,size_t n) {
 if(!lock()) return ESP_ERR_INVALID_STATE;
 esp_err_t e=i2c_master_transmit_receive(dev,&reg,1,data,n,20);xSemaphoreGive(mutex);return e;
}
esp_err_t r1_i2c_write(i2c_master_dev_handle_t dev,uint8_t reg,const uint8_t *data,size_t n) {
 uint8_t bytes[17];
 if(n>16) return ESP_ERR_INVALID_ARG;
 bytes[0]=reg;memcpy(bytes+1,data,n);
 if(!lock()) return ESP_ERR_INVALID_STATE;
 esp_err_t e=i2c_master_transmit(dev,bytes,n+1,20);xSemaphoreGive(mutex);return e;
}
