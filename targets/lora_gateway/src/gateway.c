#include "gateway.h"
#include <stddef.h>
#include <string.h>
#include "lm_msgs.h"

_Static_assert(offsetof(lm_beacon_obs_t, rssi) == sizeof(lm_beacon_payload_t), "BeaconObs = BeaconPayload + rssi");

void gateway_init(gateway_t *g, const lm_link_if_t *air, const lm_link_if_t *host, const lm_lora_cfg_t *radio) {
    memset(g, 0, sizeof *g); g->air = air; g->host = host; g->radio = radio; lm_slots_init(&g->slots);
    lm_txq_init(&g->txq, g->qbuf, GW_QUEUE);
}

static void on_air(uint8_t id, const uint8_t *p, uint8_t len, int8_t rssi, void *user) {
    gateway_t *g = user;
    if (id == LM_MSG_BEACON_PAYLOAD && len == sizeof(lm_beacon_payload_t)) {
        lm_beacon_obs_t o; memcpy(&o, p, len); o.rssi = rssi;      /* wire -> wire: payload is the obs prefix */
        lm_slot_sync(&g->slots, o.phase_ms, lm_lora_airtime_ms(g->radio, (uint8_t)(len + 6)), g->now);
        lm_send(g->host, LM_MSG_BEACON_OBS, &o, sizeof o);
    } else lm_send(g->host, id, p, len);                             /* acks, reports, polls: pass through */
}

static void on_host(uint8_t id, const uint8_t *p, uint8_t len, int8_t rssi, void *user) {
    gateway_t *g = user; (void)rssi;
    lm_txq_push(&g->txq, id, p, len);
}

void gateway_tick(gateway_t *g, uint32_t now) {
    g->now = now;
    lm_poll(g->air, on_air, g);
    lm_poll(g->host, on_host, g);
    g->now = lm_txq_flush(&g->txq, g->air, &g->slots, g->radio, g->now);
}
