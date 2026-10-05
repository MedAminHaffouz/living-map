#include "lm_txq.h"
#include <string.h>

void lm_txq_init(lm_txq_t *q, lm_txq_msg_t *buf, uint8_t cap) { q->buf = buf; q->cap = cap; q->head = q->n = 0; q->dropped = 0; }

int lm_txq_push(lm_txq_t *q, uint8_t id, const void *p, uint8_t len) {
    if (q->n == q->cap || len > LM_MAX_PAYLOAD) { q->dropped++; return -1; }
    lm_txq_msg_t *m = &q->buf[(q->head + q->n++) % q->cap];
    m->id = id; m->len = len; memcpy(m->p, p, len);
    return 0;
}

uint32_t lm_txq_flush(lm_txq_t *q, const lm_link_if_t *link, const lm_slots_t *s, const lm_lora_cfg_t *radio, uint32_t now) {
    while (q->n && lm_slot_reader_window(s, now)) {
        const lm_txq_msg_t *m = &q->buf[q->head];
        uint32_t air = lm_lora_airtime_ms(radio, (uint8_t)(m->len + 6));
        uint32_t left = (LM_SLOT_READER + 1) * LM_SLOT_MS - lm_slot_phase(s, now);
        if (air > left) break;                                       /* would spill into slot 0: next period */
        lm_send(link, m->id, m->p, m->len);
        q->head = (uint8_t)((q->head + 1) % q->cap); q->n--; now += air;
    }
    return now;
}
