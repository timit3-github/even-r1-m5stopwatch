/* SPDX-License-Identifier: BSL-1.0
 * Copyright (c) 2026 even-r1-esp32s3 contributors. */
#pragma once
#include <stdbool.h>
#include <stdint.h>
struct r1_usb_power {
 bool valid,present,lock_held,release_pending;
 uint16_t millivolts;
 int64_t sample_ms,low_since_ms;
 uint32_t failures;
};
/* Snapshot published by the PM1 task; does not access I2C. */
struct r1_usb_power r1_usb_power_get(void);
/* Hold while VIN is present; release after repeated low readings. */
void r1_usb_power_update(struct r1_usb_power *s,bool valid,
                         uint16_t mv,int64_t now_ms,uint16_t on_mv,
                         uint16_t off_mv,uint32_t release_ms);
