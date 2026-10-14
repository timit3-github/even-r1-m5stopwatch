/* SPDX-License-Identifier: BSL-1.0
 * Copyright (c) 2026 even-r1-esp32s3 contributors. */
#pragma once
/* M5Stack StopWatch defaults; set unused board pins to -1. */
#define R1_BUTTON_GPIO 1 /* blue KEYB: tap gestures, active-low */
#define R1_NAV_BUTTON_GPIO 2 /* yellow KEYA: short=down, hold=up, active-low */
#define R1_NAV_BUTTON_HOLD_MS 700
#define R1_NAV_BUTTON_REPEAT_MS 500 /* up/down repeat after hold; 0 disables */
#define R1_NAV_BUTTON_DOUBLE_MS 300 /* short release to next press */
#define R1_NAV_BUTTON_FOLLOWUP_HOLD_MS 200 /* short then hold starts down */
#define R1_BUTTON_DEBOUNCE_MS 30
#define R1_BUTTON_HOLD_MS 700
#define R1_BUTTON_DOUBLE_MS 300 /* first release to second press; single tap waits */
#define R1_BUTTON_FOLLOWUP_HOLD_MS 200 /* experimental tap-then-hold threshold */
#define R1_BUTTON_INTERVAL_TICKS 128 /* minimum button event spacing; 125ms at 1024Hz */
#define R1_TOUCH_TOGGLE_WINDOW_MS 100 /* maximum G1/G2 press separation */
#define R1_TOUCH_TOGGLE_HOLD_MS 500 /* hold both; one toggle until both released */
#define R1_HAPTIC_ENABLED 1 /* StopWatch M5IOE1 motor; set 0 for other boards */
#define R1_HAPTIC_PULSE_MS 100
#define R1_HAPTIC_GAP_MS 120
#define R1_HAPTIC_DUTY_PERCENT 60
#define R1_ENCODER_A_GPIO -1
#define R1_ENCODER_B_GPIO -1
#define R1_ENCODER_EDGES_PER_STEP 4
#define R1_ENCODER_REVERSE 0 /* set 1 to swap the two swipe directions */
#define R1_ENCODER_INTERVAL_TICKS 128 /* 125ms at G2's 1024Hz clock */
#define R1_ENCODER_PENDING_MAX 4
#define R1_POWER_HOLD_GPIO -1 /* M5Dial GPIO46 must not be driven on StopWatch */
#define R1_DISPLAY_BACKLIGHT_GPIO -1 /* StopWatch uses AMOLED, no GPIO9 backlight */
/* User-tested version; reconnect after reboot may take several minutes. */
#define R1_APP_VERSION "2.9.9.9999"
#define R1_HW_VERSION "603MV1.9.3"
/* Emulated identity; not a copied retail serial. Exactly 15 ASCII characters. */
#define R1_SERIAL "ESP32S3R1TST001"
#define R1_BATTERY_PERCENT 100 /* fallback until the first successful reading */
#define R1_BATTERY_PM1_ENABLED 1 /* StopWatch only; set 0 for M5Dial */
#define R1_BATTERY_SDA_GPIO 47
#define R1_BATTERY_SCL_GPIO 48
#define R1_BATTERY_POLL_MS 1000
#define R1_BATTERY_NOTIFY_MS 30000 /* also notify when the estimated % changes */
#define R1_BATTERY_EMPTY_MV 3300
#define R1_BATTERY_FULL_MV 4200
#define R1_TOUCH_ENABLED 1 /* StopWatch CST820B; set 0 for M5Dial */
#define R1_TOUCH_POLL_MS 20
#define R1_TOUCH_STALE_MS 300
#define R1_TOUCH_SWIPE_PX 60 /* initial swipe threshold */
#define R1_TOUCH_REPEAT_MS 500 /* fixed-direction repeat while touching; 0 disables */
#define R1_TOUCH_TAP_SLOP_PX 20 /* movement beyond this cancels tap/hold */
#define R1_TOUCH_SWIPE_REVERSE 0 /* 0: finger upward=up, downward=down */
/* Automatic power management; BLE stays enabled. */
#define R1_IDLE_TIMEOUT_MS 60000 /* 0 disables button-only user standby */
#define R1_IDLE_HOST_POLL_MS 1000
#define R1_IDLE_BATTERY_POLL_MS 30000
#define R1_PM_MAX_CPU_MHZ 160
#define R1_PM_MIN_CPU_MHZ 40
/* StopWatch USB supply detection and Light-sleep inhibition. */
#define R1_USB_VIN_POLL_MS 1000 /* Also poll during standby for USB reconnection. */
#define R1_USB_VIN_ON_MV 4000
#define R1_USB_VIN_OFF_MV 3000
#define R1_USB_VIN_MAX_MV 6500 /* Reject implausible readings, retaining lock. */
#define R1_USB_VIN_RELEASE_MS 2000 /* Repeated low readings before release. */
#define R1_REASSEMBLY_TIMEOUT_MS 5000
/* Logging-only target checks until command/peer byte order is observed.
 * Set 1 to reject a glasses role on an exact peer-address mismatch. */
#define R1_STRICT_TARGET_MATCH 0
/* Old stock has one live phone role and one live glasses role (3 link slots). */
#define R1_FIRMWARE_VERSION "0.3.2"
#define R1_TX_TIMEOUT_MS 5000
