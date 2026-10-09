#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define R1_MESSAGE_MAX 4063
#define R1_FRAGMENT_MAX 244
#define R1_FRAGMENT_PAYLOAD 239
struct r1_rx {
 uint8_t data[R1_MESSAGE_MAX]; size_t used; uint32_t crc;
 int expected; bool active;
};
enum r1_checksum_scheme { R1_CHECKSUM_COMPACT_CCITT, R1_CHECKSUM_MODEL_MODBUS,
                          R1_CHECKSUM_AMBIGUOUS };
struct r1_model {
 uint8_t module, module_version, status, command, subcommand;
 uint16_t serial; const uint8_t *payload; size_t payload_len;
 enum r1_checksum_scheme checksum_scheme;
};
uint32_t r1_crc32(const uint8_t *, size_t);
uint16_t r1_modbus(const uint8_t *, size_t);
uint16_t r1_phone_checksum(const uint8_t *, size_t);
/* Whole model with checksum bytes 10/11 treated as zero. Does not modify input. */
uint16_t r1_model_modbus(const uint8_t *, size_t);
/* 1 complete, 0 awaiting continuation, -1 rejected (state cleared). */
int r1_receive(struct r1_rx *, const uint8_t *, size_t);
bool r1_decode(const uint8_t *, size_t, struct r1_model *);
size_t r1_encode(const struct r1_model *, uint8_t, const uint8_t *, size_t, uint8_t *, size_t);
/* seq descends to zero; exact multiples of 239 include an empty terminal. */
size_t r1_fragment(const uint8_t *, size_t, size_t, uint8_t *, size_t);
