/* Writer STM app (everything but uros_app.c) on the host: uplink stub on the Pi side, stub Ra-02 on the air side.
   Beacon write: reader window, age 0, ack (id, version) or 1 + 3 retries 2 periods apart then ok=0; obs uplink; calibrate. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "app.h"
#include "uplink.h"
#include "lm_link.h"
#include "lm_slots.h"

#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); exit(1); } } while (0)

extern int up_det, up_odom, up_imu, up_obs, up_ack;
extern lm_beacon_obs_t up_last_obs; extern lm_beacon_ack_t up_last_ack;
extern int stub_lora_ntx; extern uint8_t stub_lora_tx[64][80], stub_lora_txlen[64];
void stub_lora_inject(const uint8_t *d, uint8_t n, uint8_t pkt_rssi);

static uint8_t got_id, got[64]; static int got_n;
static void grab(uint8_t id, const uint8_t *p, uint8_t len, void *ctx) { (void)ctx; got_id = id; memcpy(got, p, len); got_n++; }
static lm_beacon_payload_t tx_write(int i) {          /* decode TX packet i, must be one BeaconPayload */
    lm_decoder_t d; lm_decoder_init(&d); got_n = 0;
    for (int k = 0; k < stub_lora_txlen[i]; k++) lm_decoder_feed(&d, stub_lora_tx[i][k], grab, 0);
    CHECK(got_n == 1 && got_id == LM_MSG_BEACON_PAYLOAD);
    lm_beacon_payload_t p; memcpy(&p, got, sizeof p); return p;
}
static void inject(uint8_t id, const void *p, uint8_t n) { uint8_t f[80]; stub_lora_inject(f, (uint8_t)lm_encode(id, p, n, f), 100); }
static uint32_t at[64];                               /* board time of each send */
static void run_until(int (*done)(void), int max_ticks) {
    for (int i = 0; i < max_ticks && !done(); i++) { int n = stub_lora_ntx; app_tick(); if (stub_lora_ntx > n) at[n] = board_millis(); }
}
static int ack_seen_base;
static int got_ack(void) { return up_ack > ack_seen_base; }
static int one_send(void) { return stub_lora_ntx >= 1; }

int main(void) {
    app_init();
    for (int i = 0; i < 500; i++) app_tick();
    CHECK(up_det > 0 && up_odom > 0 && up_imu > 0 && stub_lora_ntx == 0);

    /* write with no ack at all: 1 + 3 retries, each in the reader window, >= 2 periods apart, then ok = 0 */
    lm_beacon_payload_t w = { .id = 4, .what = LM_EVENT_TYPE_FIRE, .prio = 3, .conf = 200, .version = 0, .age_s = 99 };
    ack_seen_base = up_ack; app_on_beacon_write(&w);
    run_until(got_ack, 10000);
    CHECK(up_ack == ack_seen_base + 1 && up_last_ack.id == 4 && up_last_ack.version == 0 && up_last_ack.ok == 0);
    CHECK(stub_lora_ntx == 4);
    for (int i = 0; i < 4; i++) {
        lm_beacon_payload_t p = tx_write(i);
        CHECK(p.id == 4 && p.age_s == 0 && p.what == LM_EVENT_TYPE_FIRE && p.phase_ms / LM_SLOT_MS == LM_SLOT_READER);
        if (i) CHECK(at[i] - at[i - 1] >= 2 * LM_SLOT_PERIOD_MS);
    }

    /* write acked by the beacon: forwarded as is, no retry */
    stub_lora_ntx = 0; w.id = 5; w.version = 2; ack_seen_base = up_ack; app_on_beacon_write(&w);
    run_until(one_send, 10000); CHECK(stub_lora_ntx == 1);
    lm_beacon_ack_t wrong = { 5, 1, 1 }; inject(LM_MSG_BEACON_ACK, &wrong, sizeof wrong);   /* other version: ignored */
    app_tick(); CHECK(up_ack == ack_seen_base);
    lm_beacon_ack_t a = { 5, 2, 1 }; inject(LM_MSG_BEACON_ACK, &a, sizeof a);
    app_tick(); CHECK(up_ack == ack_seen_base + 1 && up_last_ack.id == 5 && up_last_ack.ok == 1);
    for (int i = 0; i < 3000; i++) app_tick();
    CHECK(stub_lora_ntx == 1);

    /* a beacon broadcast heard -> /beacon/obs with RSSI */
    lm_beacon_payload_t b = { .id = 7, .what = LM_EVENT_TYPE_PERSON, .age_s = 30, .version = 1, .phase_ms = 1400 };
    inject(LM_MSG_BEACON_PAYLOAD, &b, sizeof b); app_tick();
    CHECK(up_obs == 1 && up_last_obs.id == 7 && up_last_obs.age_s == 30 && up_last_obs.phase_ms == 1400 && up_last_obs.rssi == -157 + 100 - 7);

    /* queue is 8 deep: the 9th write is refused at once with ok = 0 */
    ack_seen_base = up_ack;
    for (uint8_t i = 0; i < 9; i++) { w.id = (uint8_t)(10 + i); app_on_beacon_write(&w); }
    CHECK(up_ack == ack_seen_base + 1 && up_last_ack.id == 18 && up_last_ack.ok == 0);

    CHECK(app_on_calibrate(0, LM_CALIB_OP_ENCODER_RESET) == 1 && app_on_calibrate(LM_EVENT_TYPE_GAS, LM_CALIB_OP_ZERO_BASELINE) == 1);
    CHECK(app_on_calibrate(0, LM_CALIB_OP_IMU_BIAS) == 1 && app_on_calibrate(0, 99) == 0);
    app_on_cmd_vel(0.3f, 0.1f); app_on_link_lost();                 /* must not crash; motors watchdog covers the rest */
    printf("ALL OK\n");
    return 0;
}
