#include "lm_aging.h"
#include <math.h>
static float tau_s(uint8_t t) {
    switch (t) { case 1: return 120.f; case 2: return 300.f; case 3: return 900.f;
                 case 4: return 1e9f; case 5: return 180.f; default: return 300.f; }
}
float lm_age_confidence(float c0, uint32_t age_s, uint8_t t) { return c0 * expf(-(float)age_s / tau_s(t)); }
uint8_t lm_age_state(float c0, uint32_t age_s, uint8_t t, uint8_t flags) {
    if (flags & 0x01) return LM_AGE_SUSPECT;
    float c = lm_age_confidence(c0, age_s, t);
    return c >= 0.7f ? LM_AGE_FRESH : c >= 0.4f ? LM_AGE_AGING : LM_AGE_STALE;
}
