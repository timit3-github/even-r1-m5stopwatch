/* SPDX-License-Identifier: BSL-1.0
 * Copyright (c) 2026 even-r1-esp32s3 contributors. */
#pragma once
void test_haptic_log(const char *tag,const char *format,...);
#define ESP_LOGI(...) test_haptic_log(__VA_ARGS__)
#define ESP_LOGW(...) test_haptic_log(__VA_ARGS__)
#define ESP_LOGE(...) test_haptic_log(__VA_ARGS__)
