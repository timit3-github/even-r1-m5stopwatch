<!-- SPDX-License-Identifier: BSL-1.0
Copyright (c) 2026 even-r1-esp32s3 contributors. -->
# Licensing and references

Copyright (c) 2026 even-r1-esp32s3 contributors.

Original project code, tests, tools and documentation are licensed under the
Boost Software License 1.0 (BSL-1.0); the full text is in LICENSE.
Files conservatively retaining MIT reference-implementation conditions carry
`SPDX-License-Identifier: BSL-1.0 AND MIT` and the corresponding upstream
copyright notice. Both notices apply to those files; the MIT notice is not
removed by Boost's object-code exception. Keep this NOTICE and LICENSES when
redistributing the project or binaries containing those portions.
Other original files carry `SPDX-License-Identifier: BSL-1.0`.

## openCFW reference work — MIT

Copyright (c) 2026 openCFW contributors.

Protocol layouts, CRC/checksum algorithms and historical reference fixtures
were implemented using documentation and reference code from
[evenRealities-openCFW](https://github.com/kalanihelekunihi/evenRealities-openCFW/tree/832137ec).
The upstream license is preserved verbatim in LICENSES/openCFW-MIT.txt.
The conservative file-level attribution covers src/main.c, src/r1_wire.c,
src/r1_legacy.c and tests/test_wire.c.
This distribution does not contain the project's stock firmware binaries or
decompiler exports. A license on a research repository is not an assertion
that the original device manufacturer's firmware is licensed under MIT.

## M5Stack reference work — MIT

Copyright (c) 2025, 2026 M5Stack Technology CO LTD.

StopWatch battery conversion/filter behavior, touch frame decoding and IOE
register/reset behavior are based on these official reference implementations:

- [M5StopWatch-UserDemo](https://github.com/m5stack/M5StopWatch-UserDemo/tree/6b4aa125288b6fe9dca661f10159f6e1e5ee785c), hal_pmic.cpp, hal_ioe.cpp and drivers/cst820.
- [M5PM1](https://github.com/m5stack/M5PM1/tree/be9a5456c007c333e7ac963f33bfde1ffa5d82ee/src), voltage and I2C registers.
- [M5IOE1 1.0.8](https://github.com/m5stack/M5IOE1/tree/1.0.8/src), pin enumeration and register operations.

The StopWatch demo and M5IOE1 carry the 2026 copyright notice, preserved in
LICENSES/M5Stack-MIT.txt. M5PM1 carries a 2025 copyright notice, preserved
separately in LICENSES/M5PM1-MIT.txt. Both upstream MIT texts are unchanged.
Attribution covers
src/r1_battery.c, src/r1_battery_math.c, src/r1_touch.c and
src/r1_touch_gesture.c. These are small C implementations; the upstream C++
libraries and demo are not bundled or build dependencies.

## Protocol facts and external dependencies

The newer wire type9 mapping was checked against
[g2flash gesture_fwd.c](https://github.com/jimrandomh/g2flash/blob/ca7e0b7a882d50c8ec8e0e597ff93640950a655f/patches/gesture_fwd.c).
No g2flash implementation code or firmware patch is distributed here.
MentraOS R1.kt was consulted in the initial investigation; none of its
implementation is included in this release.

ESP-IDF, NimBLE and any other externally installed build/runtime dependencies
retain their own licenses. They are not included in this source archive.
tools/capture.py optionally uses the separately installed pyserial package,
which retains its own license. Review SDK/dependency notices when distributing
compiled firmware; this source review does not inventory the linked SDK.

Even Realities, M5Stack and other product names identify compatibility targets.
This is an independent, unofficial project and is not affiliated with or
endorsed by those manufacturers.
