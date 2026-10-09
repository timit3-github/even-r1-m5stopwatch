# Protocol evidence and implementation choices (v0.2.3)

The primary reference is stock R1 2.2.6.0009 and the reconstruction of stock
G2 2.2.6.10 in evenRealities-openCFW. These are not observations of the user's
app 2.3.2 / G2 2.3.2.14, except for the user-supplied endpoint log below.
No R1 or G2 hardware is directly available to the implementer.

## Confirmed console control and M5Dial extension

The user subsequently reported all console commands work with G2 2.3.2.14.
Actual logs show type1/type2 touch notifications and continued heartbeat ACKs.
M5Dial v0.2.3 maps physical quadrature detents to the same tested type4/type5
notifications, and GPIO42 button to type1/type0/type8. GPIO hardware itself is
not yet verified. Display/touch drivers are not introduced. See M5Dial_ja.md.

## G2 fixed handles and actual v0.2.1 connection

The next user log confirms phone encryption/bonding and reports successful
app pairing. A public-address peer matching advStart target 1 connects, with
no RX_CH1 or subscription before disconnect. Old G2 ble_ring_profile.c resets
handles to write=0x10, notify=0x12, CCCD=0x13 at connection open and schedules
CCCD writes. The v0.2.1 standard-service configuration shifts BAE8 two handles
higher (actual channel-2 notify 25 confirms the computed layout). v0.2.2
disables the additional GAP PPCP characteristic by setting all four configuration
values to zero. It checks actual handles at runtime and logs registration.
The current G2 ATT write itself has not been captured; success is unverified.
See UPDATE_v0.2.2_ja.md and VALIDATION.md for exact source/SDK evidence.

## Actual iPhone pairAuth request, 2026-10-09

The user's v0.2 ESP-IDF 5.5.1 hardware log contains:
`00 57 0e 23 01 64 01 64 01 00 00 00 08 0d 00 26 b1 01`.
The fragment CRC32 is 0x01230E57. The 13-byte model is module 1,
command 0, subcommand 8, serial 1, payload 01. Its checksum is 0xB126:
CRC16/MODBUS, initial 0xFFFF, reflected polynomial 0xA001, over all model
bytes with positions 10 and 11 treated as zero. The compact CCITT calculation
instead produces 0x013F. v0.2 rejected this valid frame because it only
accepted compact CCITT on receive. v0.2.1 validates both schemes and logs
the matching scheme. This one observation does not establish the scheme
for every subsequent command or successful pairing. Outgoing MODBUS is unchanged.

## Legacy 0x88: internal event, not an on-wire fixed auth byte

Old G2 sends `00 35 88 00`. Stock R1's opcode dispatcher at `0x0004E258`
calls `0x00033850(session, 2, 1, &role)` with role byte `2`.
Tracing that helper shows an internal queue envelope, not a GATT sender.
The consumer at `0x00045184` handles event 2 / role 2, checks target peers,
and invokes the battery/wear notification paths (`0x000913DC`, `0x0004C678`).
Therefore this implementation sets a single glasses role and emits those
notifications, rather than inventing an `0x88` response envelope or sending
the byte `02` directly. This corrects the earlier interpretation of this path.
The old path contains no demonstrated challenge/secret-key calculation.
This does not establish the absence of additional checks in the current app.

## Fixed old request/response contracts

| Opcode | Request length | Handler | Response length | Mutation |
|---|---:|---|---:|---|
| 85 | 8 | 00092B98 | 7 | byte 4 = 1 |
| 8A | 6 | 00062B4C | 7 | byte 4 = 1, workspace tail zero |
| 89 | 8 | 0006A714 | 7 | byte 4 = 1 |
| 94 | 4 | 0004E1F8 | 5 | byte 4 = 1 |

Stock clears its 36-byte workspace before copying the request. The 8A reply
thus has a zero at byte 6. Header byte 3 remains 1 in these SET acknowledgments;
it is not rewritten to the zero used by unsolicited touch notifications.
The responder at `0x0008967C` sends to the current glasses handle through
the channel-1 TX path. Unknown commands and GET variants are logged only.

The heartbeat ACK is `00 1A 94 01 01`. This is the stock handler's observed
mutation; values 20/40 recognized by a special G2 recovery path are not invented.

## Unsolicited notifications

- Battery: `00 09 8B 00 percent charging`. The last byte is a boolean from
  `0x00091256`, unlike the EUS deviceStatus enum (1 charging, 2 not, 3 full).
- Wear: `00 09 8C 00 wearing`.
- Touch: `00 09 61 00 type v0 v1 tick:u32LE`, 11 bytes in stock nonfactory mode.
  The producer at `0x0003E938` constructs the header word `0x00610900`.
  Types 1/2/0 correspond to tap/double/hold; 4/5 are the two swipe directions;
  8 is the terminal event. The old G2 parser dispatches type 0/1/2/4/5/8
  to input events 3/0/1/5/4/14 respectively, and reads v0/v1 for swipes.
  Swipe UI direction and movement scaling require hardware validation.
  Timestamps use 1024 Hz and the G2 parser suppresses intervals below 100 ticks
  except type 8. Synthetic timestamp increments are not used to bypass that rule.

## Connection and application policy

- At most one phone role and one glasses role; there are three physical link slots.
- The role is reacquired through phone pairAuth or channel-1 CCCD events /
  legacy requests on each connection. The old BAE8 router at `0x0005D5E0`
  assigns the glasses role for group-A events 6/7; `0x0007CF4C` generates
  these from the first notify characteristic's CCCD writes.
- Phone pairAuth replies with payload 00, then emits an unsolicited payload 00
  after encryption. The 100 ms scheduling delay is approximate, not a complete
  reproduction of stock timers or all failure notifications.
- advStart receives exactly 12 bytes, persists two six-byte targets, and logs
  exact matches. Public documentation flags first-party address byte order as
  an unresolved boundary. Default mismatch checks are diagnostic only.
- Advertising is undirected/100 ms while a role is missing, stopped when both
  roles are occupied, and resumed after disconnect. Directed and slow modes
  and delayed mismatch-disconnect are not implemented.
- EUS settings GET/SET uses status bit 1. An unimplemented command is never
  reported as successfully implemented. Result codes beyond success are unknown.
- Configuration writes simulate storage only. Health hardware, power control,
  algorithm license, factory commands and DFU are not reproduced.
- Wire logs are endpoint logs from the ESP, not a capture of an existing R1 link.

## Primary references

- [R1 protocol](https://github.com/kalanihelekunihi/evenRealities-openCFW/blob/main/r1/docs/protocol.md)
- [Legacy command dispatch](https://github.com/kalanihelekunihi/evenRealities-openCFW/blob/main/r1/docs/correlation/LEGACY-COMMAND-DISPATCH-CORRELATION.md)
- [Connection control](https://github.com/kalanihelekunihi/evenRealities-openCFW/blob/832137ec/r1/docs/correlation/CONNECTION-CONTROL-CORRELATION.md)
- [Old R1 decompiler export](https://github.com/kalanihelekunihi/evenRealities-openCFW/blob/832137ec/r1/research/decompilation/application/decompiler-output.c)
- [Old G2 receiver and sender](https://github.com/kalanihelekunihi/evenRealities-openCFW/blob/832137ec/g2/components/apollo_main/core_overlay/ring_service.c)
- [Touch switch / advStart](https://github.com/kalanihelekunihi/evenRealities-openCFW/blob/main/r1/docs/correlation/ADV-START-TOUCH-SWITCH-HANDLERS-CORRELATION.md)
- [Role assignment](https://github.com/kalanihelekunihi/evenRealities-openCFW/blob/main/r1/docs/correlation/CONNECTION-ROLE-ASSIGNMENT-CORRELATION.md)
- [Peer target policy](https://github.com/kalanihelekunihi/evenRealities-openCFW/blob/main/r1/docs/correlation/PEER-TARGET-POLICY-CORRELATION.md)
- [Physical R1 2.2.8 validation](https://github.com/kalanihelekunihi/evenRealities-openCFW/blob/main/r1/docs/closures/AUGUST-18-R1-B56EE2-HARDWARE-VALIDATION.md)

Source URLs refer to externally hosted research; this package does not include
stock firmware binaries, another device's bonding keys, or a retail device serial.
