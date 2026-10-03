#include "sensor_node.h"
#include <string.h>

static float median5(const float *w, uint8_t n) {
    float a[5]; memcpy(a, w, n * sizeof(float));
    for (uint8_t i = 1; i < n; i++) { float x = a[i]; int j = i - 1; while (j >= 0 && a[j] > x) { a[j + 1] = a[j]; j--; } a[j + 1] = x; }
    return a[n / 2];
}
static uint8_t popcount8(uint8_t v) { uint8_t c = 0; while (v) { c += v & 1u; v >>= 1; } return c; }

void sensor_node_init(sensor_node_t *s, const sensor_cfg_t *cfg, uint32_t now_ms) {
    memset(s, 0, sizeof *s); s->cfg = cfg; s->t_start = now_ms;
}
int sensor_node_tick(sensor_node_t *s, uint32_t now_ms, lm_sensor_det_t *out) {
    const sensor_cfg_t *c = s->cfg;
    if (now_ms - s->t_last < c->period_ms) return 0;
    s->t_last = now_ms;
    s->win[s->wi] = c->read(); s->wi = (s->wi + 1) % 5; if (s->wfill < 5) s->wfill++;
    float v = median5(s->win, s->wfill);
    uint8_t above = s->detected ? (v > c->th_off) : (v > c->th_on);           /* hysteresis */
    s->hist = (uint8_t)((s->hist << 1) | above); if (s->hn < c->n) s->hn++;
    uint8_t mask = (uint8_t)((1u << c->n) - 1u);
    uint8_t warm = (now_ms - s->t_start) >= c->warmup_ms;
    if (warm && s->hn >= c->n) s->detected = popcount8(s->hist & mask) >= c->k;  /* debounce */
    out->type = c->type; out->detected = s->detected; out->value = v;
    out->conf = warm ? 230 : 0; out->t_ms = now_ms;                            /* TODO: health-based conf */
    return 1;
}
