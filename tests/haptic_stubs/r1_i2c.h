/* SPDX-License-Identifier: BSL-1.0
 * Copyright (c) 2026 even-r1-esp32s3 contributors. */
#pragma once
#include "esp_pm.h"
#include <stddef.h>
#include <stdint.h>
typedef void *i2c_master_dev_handle_t;
esp_err_t r1_i2c_add(uint8_t address,uint32_t hz,i2c_master_dev_handle_t *out);
esp_err_t r1_i2c_remove(i2c_master_dev_handle_t device);
esp_err_t r1_i2c_probe(uint8_t address);
esp_err_t r1_i2c_read(i2c_master_dev_handle_t device,uint8_t reg,uint8_t *data,size_t size);
esp_err_t r1_i2c_write(i2c_master_dev_handle_t device,uint8_t reg,const uint8_t *data,size_t size);
esp_err_t r1_i2c_update16(i2c_master_dev_handle_t device,uint8_t reg,uint16_t set,uint16_t clear);
