/* Framing shared by every firmware target. Mirror of libs/lm_core/link.py. */
#pragma once
#include <stdint.h>
#include <stddef.h>
#define LM_SYNC0 0xA5
#define LM_SYNC1 0x5A
#define LM_MAX_PAYLOAD 64

uint16_t lm_crc16(const uint8_t *d, size_t n, uint16_t crc);
/* returns frame length written to out (needs len+6 bytes) */
size_t lm_encode(uint8_t id, const void *payload, uint8_t len, uint8_t *out);

typedef struct { uint8_t st, id, len, idx; uint8_t buf[LM_MAX_PAYLOAD]; uint16_t crc; } lm_decoder_t;
typedef void (*lm_on_msg)(uint8_t id, const uint8_t *payload, uint8_t len, void *ctx);
void lm_decoder_init(lm_decoder_t *d);
/* feed one byte (call from UART RX ISR/DMA loop); calls cb on every valid frame */
void lm_decoder_feed(lm_decoder_t *d, uint8_t b, lm_on_msg cb, void *ctx);
