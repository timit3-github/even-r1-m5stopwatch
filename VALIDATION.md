# Validation — v0.2.3, 2026-10-09

## User-confirmed hardware baseline: v0.2.2

User reports all console commands work with G2 2.3.2.14. Endpoint logs confirm
GATT layout, channel-1 subscription, glasses role, 88/85/89 requests and replies,
continued 94 heartbeat, and 11-byte type1/type2 operation notifications. iPhone
app 2.3.2 pairing and actual G2 UI response are user-confirmed. This establishes
the existing BLE operation path, not the new physical GPIO input implementation.

## New M5Dial inputs: host tests passed

Quadrature forward/reverse fixed sequences, full detents, contact bounce,
partial reversal, diagonal transition reset, repeated level, two-edge setting.
Button contact debounce, short release click, 700ms hold once, hold release
without click, boot-held suppression, uint32 timer wrap and delayed polling.
C11 -Wall -Wextra -Werror for wire/legacy/input suites. Input suite passed
AddressSanitizer + UndefinedBehaviorSanitizer with leak detection disabled.

M5Dial official documentation checked for A=G41, B=G40, 16 detents/64 pulses,
GPIO46 power hold and GPIO9 LCD backlight; GPIO42 BtnA documented by M5Unified.
GPIO ISR uses only phase sampling and FreeRTOS queue send, no BLE. Host owns
all protocol state and physical event scheduling. Overflow resynchronizes and
reports dropped steps. Non-IRAM ISR can miss edges during flash operations.

## Not yet verified

No v0.2.3 full ESP-IDF cross-build or physical GPIO trial was possible here;
the ESP-IDF/toolchain install is absent. No binaries are included. Build with
user's existing ESP-IDF 5.5.1 environment. Rotation direction, one-click scaling,
physical contact quality and power hold/backlight behavior require M5Dial test.
See M5Dial_ja.md for steps and configuration.

---
Historical v0.2.2 validation follows for provenance; its earlier unverified
G2-control limitation is superseded by the user-confirmed baseline above.

## Historical validation — v0.2.2, 2026-10-09

## Hardware evidence: v0.2.1

User reports iPhone pairing completed. The actual endpoint log confirms phone
role, encrypted=1, bonded=1, successful pairAuth TX, status/time/settings replies
and advStart. The public-address second connection matches target index 1;
it is likely G2. It has no channel-1 subscription/write before remote disconnect.
No successful G2 control is established.

## GATT layout source verification

IDF v5.5.1 submodule metadata pins esp-nimble to
b45dcedcafb7888174c3567002c36b342ec0b723. Its gap/gatt service sources and
public ble_gatt.h were inspected. Under the user's configuration, GAP has
7 attributes (name, appearance, PPCP); GATT has 8 (Service Changed + CCCD,
Server Supported Features, Client Supported Features; caching disabled).
Removing PPCP leaves 5+8 attributes before the BAE8 service, making channel-1
RX/TX/CCCD 0x10/0x12/0x13 and channel-2 RX/TX/CCCD 0x15/0x17/0x18.
The v0.2.1 computed channel-2 TX value 0x19 matches actual att_handle=25.
Old G2 ble_ring_profile.c resets these fixed handles at connection open and
writes the CCCD after 500/700/900ms. Current G2 ATT requests are not captured;
this remains a strong hypothesis to validate by the new runtime layout logs
and actual SUBSCRIBE/RX_CH1 after the change.

v0.2.2 uses public registration callbacks and ble_gatts_find_dsc to report/check
the actual database, refusing advertising on mismatch. UUIDs/properties and
role assignment are unchanged. Service Changed is queued after encrypted,
bonded connection events; delivery depends on the peer subscribing.

## Tests: passed

Wire/legacy C11 -Wall -Wextra -Werror host suites pass, including the actual
MODBUS iPhone pairAuth oracle and historical compact CCITT compatibility.
The sdkconfig updater was tested with CRLF, unrelated settings, backup
preservation, repeated invocation, wrong target and missing-key rejection.
Its original file remains unchanged when validation fails.

## Limitations

No v0.2.2 ESP32-S3 cross-build or hardware trial was performed here. The SDK and
cross compiler are unavailable in this environment; preceding retrieval attempts
rejected corrupt package downloads and encountered mirror timeouts. No old
binaries are included. Use the user's existing ESP-IDF 5.5.1 build environment.
The new startup logs verify the actual runtime table rather than assuming every
SDK version has identical built-in service sizes. See UPDATE_v0.2.2_ja.md.

## Primary source locations

- https://github.com/espressif/esp-idf/tree/v5.5.1/components/bt/host/nimble
- https://github.com/espressif/esp-nimble/blob/b45dcedcafb7888174c3567002c36b342ec0b723/nimble/host/services/gap/src/ble_svc_gap.c
- https://github.com/espressif/esp-nimble/blob/b45dcedcafb7888174c3567002c36b342ec0b723/nimble/host/services/gatt/src/ble_svc_gatt.c
- https://github.com/kalanihelekunihi/evenRealities-openCFW/blob/832137ec/g2/components/apollo_main/core_overlay/ble_ring_profile.c
