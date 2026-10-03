#include "lm_link.h"
#include <string.h>

uint16_t lm_crc16(const uint8_t *d, size_t n, uint16_t crc) {
    while (n--) { crc ^= (uint16_t)(*d++) << 8;
        for (int i = 0; i < 8; i++) crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1); }
    return crc;
}
size_t lm_encode(uint8_t id, const void *payload, uint8_t len, uint8_t *out) {
    out[0] = LM_SYNC0; out[1] = LM_SYNC1; out[2] = id; out[3] = len;
    memcpy(out + 4, payload, len);
    uint16_t c = lm_crc16(out + 2, (size_t)len + 2, 0xFFFF);
    out[4 + len] = (uint8_t)(c & 0xFF); out[5 + len] = (uint8_t)(c >> 8);
    return (size_t)len + 6;
}
void lm_decoder_init(lm_decoder_t *d) { memset(d, 0, sizeof *d); }
void lm_decoder_feed(lm_decoder_t *d, uint8_t b, lm_on_msg cb, void *ctx) {
    switch (d->st) {
    case 0: d->st = (b == LM_SYNC0); break;
    case 1: d->st = (b == LM_SYNC1) ? 2 : (b == LM_SYNC0); break;
    case 2: d->id = b; d->st = 3; break;
    case 3: d->len = b; d->idx = 0; d->st = (b > LM_MAX_PAYLOAD) ? 0 : (b ? 4 : 5); break;
    case 4: d->buf[d->idx++] = b; if (d->idx == d->len) d->st = 5; break;
    case 5: d->crc = b; d->st = 6; break;
    case 6: {
        d->crc |= (uint16_t)b << 8;
        uint8_t hdr[2] = { d->id, d->len };
        uint16_t c = lm_crc16(d->buf, d->len, lm_crc16(hdr, 2, 0xFFFF));
        if (c == d->crc && cb) cb(d->id, d->buf, d->len, ctx);
        d->st = 0; break; }
    }
}
