# SPDX-License-Identifier: BSL-1.0
# Copyright (c) 2026 even-r1-esp32s3 contributors.
idf_component_register(SRCS "main.c" "r1_wire.c" "r1_legacy.c" "r1_inputs.c" "r1_battery.c" "r1_battery_math.c" "r1_i2c.c" "r1_touch.c" "r1_touch_gesture.c" INCLUDE_DIRS "." "../include" REQUIRES bt nvs_flash driver esp_timer esp_driver_i2c)
