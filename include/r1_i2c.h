/* SPDX-License-Identifier: BSL-1.0
 * Copyright (c) 2026 even-r1-esp32s3 contributors. */
#pragma once
#include "driver/i2c_master.h"
/* Internal StopWatch bus. All add/remove/probe/transactions are serialized. */
void r1_i2c_init(void);
esp_err_t r1_i2c_add(uint8_t addr,uint32_t hz,i2c_master_dev_handle_t *dev);
esp_err_t r1_i2c_remove(i2c_master_dev_handle_t dev);
esp_err_t r1_i2c_probe(uint8_t addr);
esp_err_t r1_i2c_read(i2c_master_dev_handle_t dev,uint8_t reg,uint8_t *data,size_t n);
esp_err_t r1_i2c_write(i2c_master_dev_handle_t dev,uint8_t reg,const uint8_t *data,size_t n);
