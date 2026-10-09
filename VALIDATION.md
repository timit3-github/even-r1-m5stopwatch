# Validation — v0.2, 2026-10-09

## ESP32-S3 build: passed

PlatformIO Core 6.2.0, platform espressif32 6.12.0, ESP-IDF 5.5.0,
Xtensa GCC 14.2.0+20241119, generic ESP32-S3 DevKitC-1 configuration.
Flash is configured as 4 MB / DIO / 40 MHz, with no PSRAM.

Final build compiled main.c, r1_wire.c and r1_legacy.c, linked the firmware,
and generated bootloader.bin, partitions.bin and firmware.bin successfully.
No compiler warnings were emitted in the final build log.

- Static RAM reported by PlatformIO: 47,204 / 327,680 bytes (14.4%).
- Flash program size: 561,813 / 1,048,576-byte app partition (53.6%).
- App image file size: 562,224 bytes.
- esptool image_info validated the app image checksum and embedded SHA-256.
- Flash offsets checked against the actual generated flasher_args.json:
  bootloader 0x0, partition table 0x8000, application 0x10000.
- Generated sdkconfig checked: peripheral-only (central disabled), three links,
  preferred MTU 247, Coded PHY enabled, USB Serial/JTAG console,
  GAP PPCP 12..24 / latency 4 / timeout 600.

Build command in this execution environment:

```sh
IDF_COMPONENT_MANAGER=0 python3 -m platformio run -d even-r1-esp32s3
```

The component manager was disabled only because this execution container's
process view caused psutil.NoSuchProcess during its CMake parent-process lookup.
This project has no managed component dependencies. The normal user build
command remains `pio run`. Package downloads that failed checksum verification
were rejected and retried through working mirrors; integrity checks were not disabled.

Firmware compilation also identified and fixed the old prototype's 16-character
serial constant (now exactly 15) and a misleading-indentation compiler error.

## Protocol tests: passed

- C11 compilation with -Wall -Wextra -Werror.
- wire: independent CRC-32 formulation, MODBUS check value, exact historical
  pair-role-phone fixture, compact CCITT, ring reply checksum, fragmentation,
  exact 239-byte terminal behavior, corruption and sequence bounds.
- legacy: fixed old G2 requests and stock R1 responses for 85 / 8A / 89 / 94;
  no synthetic 88 reply; exact 11-byte 61 event; short/trailing/invalid command
  rejection; zero/erased targets; first/second/mismatch address comparisons.
- AddressSanitizer and UndefinedBehaviorSanitizer passed for wire and legacy
  tests. Leak detection disabled because the container restricts /proc inspection.

## PC tools: passed

- capture.py Python compilation and pseudo-terminal integration: command
  transmission, timestamped RX/TX file logging, Ctrl+C completion.
- flash_prebuilt.py mocked subprocess check: correct chip, port, image files,
  offsets, and absence of a full-flash erase command. No hardware was flashed.

## Not validated on hardware

- Boot, native USB console, actual advertising, link count, PHY negotiation,
  bonding persistence or notification delivery on an ESP32-S3.
- Even iPhone app 2.3.2 registration or account association.
- G2 2.3.2.14 connection, operation mapping or reconnection.
- The old source analysis assumes protocol compatibility that must be tested.

The included prebuilt images are the successful build outputs, not a claim of
successful R1 replacement. See FLASH_ja.md and README_ja.md for the first trial.
