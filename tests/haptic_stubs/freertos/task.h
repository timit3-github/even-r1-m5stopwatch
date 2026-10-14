/* SPDX-License-Identifier: BSL-1.0
 * Copyright (c) 2026 even-r1-esp32s3 contributors. */
#pragma once
#include "FreeRTOS.h"
typedef void (*TaskFunction_t)(void *);
typedef void *TaskHandle_t;
BaseType_t xTaskCreate(TaskFunction_t task,const char *name,unsigned stack,void *arg,unsigned priority,TaskHandle_t *out);
void vTaskDelay(TickType_t ticks);
