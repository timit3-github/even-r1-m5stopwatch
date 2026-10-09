#pragma once
/* M5Dial defaults; set board pins to -1 for a generic ESP32-S3. */
#define R1_BUTTON_GPIO 42 /* active-low, internal pull-up */
#define R1_BUTTON_DEBOUNCE_MS 30
#define R1_BUTTON_HOLD_MS 700
#define R1_BUTTON_DOUBLE_MS 300 /* first release to second press; single tap waits */
#define R1_BUTTON_FOLLOWUP_HOLD_MS 200 /* experimental tap-then-hold threshold */
#define R1_BUTTON_INTERVAL_TICKS 128 /* experimental spacing; 125ms at 1024Hz */
#define R1_ENCODER_A_GPIO 41
#define R1_ENCODER_B_GPIO 40
#define R1_ENCODER_EDGES_PER_STEP 4
#define R1_ENCODER_REVERSE 0 /* set 1 to swap the two swipe directions */
#define R1_ENCODER_INTERVAL_TICKS 128 /* 125ms at G2's 1024Hz clock */
#define R1_ENCODER_PENDING_MAX 4
#define R1_POWER_HOLD_GPIO 46
#define R1_DISPLAY_BACKLIGHT_GPIO 9 /* held low; display/touch not initialized */
#define R1_APP_VERSION "2.2.6.0009"
#define R1_HW_VERSION "603MV1.9.3"
/* Emulated identity; not a copied retail serial. Exactly 15 ASCII characters. */
#define R1_SERIAL "ESP32S3R1TST001"
#define R1_BATTERY_PERCENT 100
#define R1_REASSEMBLY_TIMEOUT_MS 5000
/* Logging-only target checks until command/peer byte order is observed.
 * Set 1 to reject a glasses role on an exact peer-address mismatch. */
#define R1_STRICT_TARGET_MATCH 0
/* Old stock has one live phone role and one live glasses role (3 link slots). */
#define R1_PROBE_VERSION "0.2.4"
#define R1_TX_TIMEOUT_MS 5000
