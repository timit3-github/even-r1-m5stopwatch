#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum r1_legacy_action {
    R1_LEGACY_UNSUPPORTED, R1_LEGACY_PAIR_GLASSES,
    R1_LEGACY_TOUCH_ENABLE, R1_LEGACY_HOLD_TIME,
    R1_LEGACY_GLASSES_STATUS, R1_LEGACY_HEARTBEAT
};
struct r1_legacy_plan {
    enum r1_legacy_action action;
    uint8_t response[7];
    size_t response_len;
    bool enabled;
    uint16_t hold_time_be;
    uint8_t glasses_status;
};
/* Bounded version of old stock handlers; false means malformed known request.
 * 0x88 generates an INTERNAL role event, not a fabricated 0x88 notification. */
bool r1_legacy_parse(const uint8_t *, size_t, struct r1_legacy_plan *);
/* Stock nonfactory touch notification: 00 09 61 00 type v0 v1 tick:u32LE. */
size_t r1_legacy_touch(uint8_t type, uint8_t v0, uint8_t v1, uint32_t tick,
                       uint8_t *, size_t);
bool r1_target_valid(const uint8_t address[6]);
/* -1: targets absent; 0/1: exact matching target; -2: mismatch. No byte reversal. */
int r1_target_match(const uint8_t targets[12], const uint8_t peer[6]);
