#pragma once
/* Board-independent defaults. Serial input needs no external wiring. */
#define R1_BUTTON_GPIO (-1) /* optional active-low switch to GND; choose a free pin */
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
#define R1_PROBE_VERSION "0.2"
#define R1_TX_TIMEOUT_MS 5000
