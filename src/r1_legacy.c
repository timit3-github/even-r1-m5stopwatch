#include "r1_legacy.h"
#include <string.h>

bool r1_legacy_parse(const uint8_t *p, size_t n, struct r1_legacy_plan *out) {
    if (!p || !out || n < 4 || n > 36) return false;
    *out = (struct r1_legacy_plan){0};
    size_t need;
    switch (p[2]) {
    case 0x88:
        if (n != 4 || p[0] || p[1] != 0x35 || p[3]) return false;
        out->action = R1_LEGACY_PAIR_GLASSES;
        return true;
    case 0x85: need = 8; out->action = R1_LEGACY_TOUCH_ENABLE; break;
    case 0x8a: need = 6; out->action = R1_LEGACY_HOLD_TIME; break;
    case 0x89: need = 8; out->action = R1_LEGACY_GLASSES_STATUS; break;
    case 0x94: need = 4; out->action = R1_LEGACY_HEARTBEAT; break;
    default: return true; /* preserve unknown commands in raw logs only */
    }
    if (n != need || p[0] || p[1] != 0x1a || p[3] != 1) return false;
    if (out->action == R1_LEGACY_TOUCH_ENABLE) out->enabled = p[4] != 0xff;
    if (out->action == R1_LEGACY_HOLD_TIME)
        out->hold_time_be = (uint16_t)p[4] << 8 | p[5];
    if (out->action == R1_LEGACY_GLASSES_STATUS) out->glasses_status = p[4];
    out->response_len = out->action == R1_LEGACY_HEARTBEAT ? 5 : 7;
    size_t copy = n < out->response_len ? n : out->response_len;
    memcpy(out->response, p, copy);
    out->response[4] = 1;
    return true;
}

size_t r1_legacy_touch(uint8_t type, uint8_t v0, uint8_t v1, uint32_t tick,
                       uint8_t *out, size_t cap) {
    if (!out || cap < 11 || (type != 0 && type != 1 && type != 2 &&
                            type != 4 && type != 5 && type != 8)) return 0;
    const uint8_t header[7] = {0, 9, 0x61, 0, type, v0, v1};
    memcpy(out, header, sizeof(header));
    for (unsigned i = 0; i < 4; i++) out[7+i] = (uint8_t)(tick >> (8*i));
    return 11;
}

bool r1_target_valid(const uint8_t address[6]) {
    if (!address) return false;
    bool zero = true, erased = true;
    for (unsigned i = 0; i < 6; i++) {
        zero &= address[i] == 0;
        erased &= address[i] == 0xff;
    }
    return !zero && !erased;
}
int r1_target_match(const uint8_t targets[12], const uint8_t peer[6]) {
    if (!targets || !peer) return -2;
    bool valid = false;
    for (int i = 0; i < 2; i++) {
        if (!r1_target_valid(targets+6*i)) continue;
        valid = true;
        if (!memcmp(targets+6*i, peer, 6)) return i;
    }
    return valid ? -2 : -1;
}
