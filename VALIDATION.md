# Validation — v0.2.1, 2026-10-09

## Fixed-vector host tests: passed

C11 with -Wall -Wextra -Werror; AddressSanitizer and UndefinedBehaviorSanitizer
for both wire and legacy test suites (leak detection disabled).

The actual user-supplied 18-byte iPhone pairAuth frame is a fixed oracle:
outer CRC32 0x01230E57, whole-model MODBUS 0xB126, compact CCITT 0x013F.
It now decodes to module 1 / command 0 / subcommand 8 / serial 1 / payload 01.
The checksum helper does not mutate the input. A changed payload with valid
regenerated outer CRC is still rejected by the inner CRC. Checksum, length,
header corruption and null decode arguments are rejected. The historical
compact CCITT fixture remains accepted. Fragmentation and legacy tests pass.

## Hardware evidence: v0.2 only

The user's ESP-IDF 5.5.1 log confirms boot, advertising, one peer connection,
2M PHY, MTU 247, channel-2 subscription and receipt of pairAuth. v0.2 then
rejects its MODBUS checksum. No registration, encryption or G2 control success
is established by that log. v0.2.1 has not yet been flashed on hardware.

## Firmware build: pending for v0.2.1

The current environment no longer has the previous ESP-IDF/toolchain install.
PlatformIO installation succeeded, but platform/SDK retrieval did not complete
before this source delivery. No v0.2.1 ESP32-S3 build is claimed. The archived
v0.2 firmware is excluded from this source package to prevent accidental reuse.
Build with the user's existing ESP-IDF 5.5.1 environment, preserving board
configuration. See FLASH_ja.md and UPDATE_v0.2.1_ja.md.
