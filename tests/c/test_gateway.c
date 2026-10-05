/* Host test for targets/lora_gateway/src/gateway.c with fake air/host links. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gateway.h"
#include "lm_msgs.h"

#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); exit(1); } } while (0)

typedef struct { uint8_t id, len, p[LM_MAX_PAYLOAD]; int8_t rssi; uint16_t phase; } fmsg_t;
typedef struct { fmsg_t in[32]; int nin; fmsg_t out[64]; int nout; } fake_link_t;

static gateway_t gw;
static int f_send(void *ctx, uint8_t id, const void *p, uint8_t len) {
    fake_link_t *f = ctx; CHECK(f->nout < 64);
    fmsg_t *m = &f->out[f->nout++]; m->id = id; m->len = len; memcpy(m->p, p, len);
    m->phase = lm_slot_phase(&gw.slots, gw.now);
    return 0;
}
static void f_poll(void *ctx, lm_link_rx_cb cb, void *user) {
    fake_link_t *f = ctx;
    for (int i = 0; i < f->nin; i++) cb(f->in[i].id, f->in[i].p, f->in[i].len, f->in[i].rssi, user);
    f->nin = 0;
}
static void push(fake_link_t *f, uint8_t id, const void *p, uint8_t len, int8_t rssi) {
    fmsg_t *m = &f->in[f->nin++]; m->id = id; m->len = len; m->rssi = rssi; memcpy(m->p, p, len);
}

static fake_link_t A, H;
static const lm_link_if_t AIR = { &A, f_send, f_poll }, HOST = { &H, f_send, f_poll };
static const lm_lora_cfg_t RADIO = LM_LORA_CFG_DEFAULT;
static uint32_t now = 10000;
static void run_ms(uint32_t ms) { for (uint32_t end = now + ms; now < end; now += 10) gateway_tick(&gw, now); }

static void check_sent_in_window(int from) {
    for (int i = from; i < A.nout; i++) {
        uint32_t air = lm_lora_airtime_ms(&RADIO, (uint8_t)(A.out[i].len + 6));
        CHECK(A.out[i].phase >= LM_SLOT_READER * LM_SLOT_MS && A.out[i].phase + air <= LM_SLOT_PERIOD_MS);
    }
}

int main(void) {
    gateway_init(&gw, &AIR, &HOST, &RADIO);

    /* air -> host: BeaconPayload => BeaconObs + RSSI, and phase sync */
    lm_beacon_payload_t pl = { .id = 5, .what = LM_EVENT_TYPE_GAS, .prio = 2, .conf = 99, .dir_deg = 90, .dist_cm = 300,
                               .prev_id = 4, .next_id = 255, .age_s = 12, .version = 3, .flags = 1, .phase_ms = 1000 };
    push(&A, LM_MSG_BEACON_PAYLOAD, &pl, sizeof pl, -70); gateway_tick(&gw, now);
    CHECK(H.nout == 1 && H.out[0].id == LM_MSG_BEACON_OBS && H.out[0].len == sizeof(lm_beacon_obs_t));
    lm_beacon_obs_t o; memcpy(&o, H.out[0].p, sizeof o);
    CHECK(o.id == 5 && o.what == LM_EVENT_TYPE_GAS && o.dist_cm == 300 && o.age_s == 12 && o.version == 3 &&
          o.flags == 1 && o.phase_ms == 1000 && o.rssi == -70);
    CHECK(gw.slots.synced && lm_slot_phase(&gw.slots, now) == 1000 + lm_lora_airtime_ms(&RADIO, sizeof pl + 6));

    /* air -> host: anything else passes through untouched */
    lm_beacon_ack_t ack = { 5, 3, 1 };
    push(&A, LM_MSG_BEACON_ACK, &ack, sizeof ack, -80); gateway_tick(&gw, now);
    CHECK(H.nout == 2 && H.out[1].id == LM_MSG_BEACON_ACK && H.out[1].len == sizeof ack && !memcmp(H.out[1].p, &ack, sizeof ack));
    CHECK(A.nout == 0);                                             /* nothing goes back on air */

    /* host -> air: held until the reader window, then sent in order, never spilling past it */
    lm_brief_step_t s = { 9, 0, 2, LM_ACTION_EXTINGUISH, LM_EVENT_TYPE_FIRE };
    for (uint8_t i = 0; i < 3; i++) { s.idx = i; push(&H, LM_MSG_BRIEF_STEP, &s, sizeof s, 0); }
    gateway_tick(&gw, now); CHECK(A.nout == 0 && gw.n == 3);
    run_ms(LM_SLOT_PERIOD_MS);
    CHECK(A.nout == 3 && gw.n == 0);
    for (int i = 0; i < 3; i++) CHECK(A.out[i].id == LM_MSG_BRIEF_STEP && A.out[i].p[2] == i);   /* idx in order */
    check_sent_in_window(0);

    /* queue is 16 deep: overflow drops the newest, the rest drains over a few windows */
    for (uint8_t i = 0; i < 20; i++) { s.idx = i; push(&H, LM_MSG_BRIEF_STEP, &s, sizeof s, 0); }
    gateway_tick(&gw, now); CHECK(gw.n == 16 && gw.dropped == 4);
    int from = A.nout; run_ms(8 * LM_SLOT_PERIOD_MS);
    CHECK(A.nout - from == 16 && gw.n == 0);
    for (int i = 0; i < 16; i++) CHECK(A.out[from + i].p[2] == i);
    check_sent_in_window(from);

    printf("ALL OK\n");
    return 0;
}
