#include "lm_beacon_core.h"
#include <string.h>

void lm_beacon_init(lm_beacon_t *b, const lm_beacon_cfg_t *cfg, const lm_beacon_payload_t *restored, uint32_t now) {
    memset(b, 0, sizeof *b); b->cfg = *cfg; lm_slots_init(&b->slots); b->now = now;
    if (restored && restored->id == cfg->id) { b->pl = *restored; b->written = 1; b->written_at = now - restored->age_s * 1000u; }
}

static void broadcast(lm_beacon_t *b) {
    lm_beacon_payload_t out = b->pl;
    uint32_t age = (b->now - b->written_at) / 1000u;
    out.age_s = age ? age : 1;                                  /* age 0 is reserved for writes */
    out.phase_ms = lm_slot_phase(&b->slots, b->now);
    lm_send(b->cfg.link, LM_MSG_BEACON_PAYLOAD, &out, sizeof out);
}

static void on_rx(uint8_t id, const uint8_t *p, uint8_t len, int8_t rssi, void *user) {
    lm_beacon_t *b = user; (void)rssi;
    if (id == LM_MSG_BEACON_PAYLOAD && len == sizeof(lm_beacon_payload_t)) {
        lm_beacon_payload_t in; memcpy(&in, p, len);           /* wire bytes -> packed wire struct */
        lm_slot_sync(&b->slots, in.phase_ms, b->cfg.airtime_ms, b->now);
        if (in.id != b->cfg.id || in.age_s != 0) return;       /* someone else's broadcast */
        uint8_t ok = !b->written || in.version >= b->pl.version;
        if (ok) {
            b->pl = in; b->written = 1; b->written_at = b->now;
            if (b->cfg.persist) b->cfg.persist(&b->pl, b->cfg.persist_ctx);
        }
        lm_beacon_ack_t a = { in.id, in.version, ok };
        lm_send(b->cfg.link, LM_MSG_BEACON_ACK, &a, sizeof a);
    } else if (id == LM_MSG_BEACON_POLL && len == sizeof(lm_beacon_poll_t)) {
        lm_beacon_poll_t q; memcpy(&q, p, len);
        if (q.id != b->cfg.id) return;
        b->poll_pending = 1;
        b->poll_slot = (uint8_t)((lm_slot_current(&b->slots, b->now) + 1) % LM_SLOT_COUNT);
    }
}

void lm_beacon_tick(lm_beacon_t *b, uint32_t now) {
    b->now = now;
    lm_poll(b->cfg.link, on_rx, b);
    if (!b->written) return;
    uint8_t slot = lm_slot_current(&b->slots, now), due = 0;
    if (slot != lm_slot_of_beacon(b->cfg.id)) b->sent_in_slot = 0;
    else if (!b->sent_in_slot) { due = 1; b->sent_in_slot = 1; }            /* once per period, in my slot */
    if (b->poll_pending && slot == b->poll_slot) { due = 1; b->poll_pending = 0; }   /* BeaconPoll: next slot */
    if (due) broadcast(b);
}
