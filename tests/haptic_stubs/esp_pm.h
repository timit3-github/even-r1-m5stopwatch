/* SPDX-License-Identifier: BSL-1.0
 * Copyright (c) 2026 even-r1-esp32s3 contributors. */
#pragma once
#include <assert.h>
typedef int esp_err_t;
typedef void *esp_pm_lock_handle_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_TIMEOUT -2
#define ESP_PM_NO_LIGHT_SLEEP 1
#define ESP_ERROR_CHECK(e) assert((e)==ESP_OK)
const char *esp_err_to_name(esp_err_t error);
esp_err_t esp_pm_lock_create(int type,int arg,const char *name,esp_pm_lock_handle_t *out);
esp_err_t esp_pm_lock_acquire(esp_pm_lock_handle_t lock);
esp_err_t esp_pm_lock_release(esp_pm_lock_handle_t lock);
esp_err_t esp_pm_lock_delete(esp_pm_lock_handle_t lock);
