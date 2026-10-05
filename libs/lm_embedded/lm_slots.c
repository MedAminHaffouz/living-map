#include "lm_slots.h"

#define P LM_SLOT_PERIOD_MS
static int32_t mod_p(int64_t x) { int32_t m = (int32_t)(x % P); return m < 0 ? m + P : m; }

void lm_slots_init(lm_slots_t *s) { s->offset_ms = 0; s->synced = 0; }
uint16_t lm_slot_phase(const lm_slots_t *s, uint32_t now) { return (uint16_t)mod_p((int64_t)now + s->offset_ms); }
uint8_t lm_slot_current(const lm_slots_t *s, uint32_t now) { return (uint8_t)(lm_slot_phase(s, now) / LM_SLOT_MS); }

void lm_slot_sync(lm_slots_t *s, uint16_t rx_phase, uint32_t airtime_ms, uint32_t now) {
    int32_t err = mod_p((int64_t)rx_phase + airtime_ms - lm_slot_phase(s, now));   /* 0..P-1 */
    if (err >= P / 2) err -= P;                                                       /* -> [-P/2, P/2) */
    s->offset_ms = mod_p((int64_t)s->offset_ms + (s->synced ? err / 4 : err));
    s->synced = 1;
}
