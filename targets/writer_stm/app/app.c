#include "app.h"
#include <string.h>
#include "lm_msgs.h"
#include "lm_link_if.h"
#include "lm_slots.h"
#include "uplink.h"
#include "sensor_node.h"
#include "odometry.h"
#include "motors.h"
#include "dropper.h"

/* sensor nodes = config only (see sensor_node.h) */
static const sensor_cfg_t CFG_GAS  = { LM_EVENT_TYPE_GAS,  board_read_gas_ppm,     200.f, 120.f, 3, 5, 60000, 100 };
static const sensor_cfg_t CFG_TEMP = { LM_EVENT_TYPE_FIRE, board_read_temp_c,      55.f,  45.f,  3, 5, 0,     100 };
static const sensor_cfg_t CFG_SMOKE= { LM_EVENT_TYPE_SMOKE,board_read_smoke_ratio, 1.8f,  1.4f,  3, 5, 20000, 100 };
static const sensor_cfg_t *CFGS[] = { &CFG_GAS, &CFG_TEMP, &CFG_SMOKE };
#define N_SENS (sizeof CFGS / sizeof *CFGS)
static sensor_node_t sens[N_SENS];
static odom_t odom; static uint32_t t_odom, now;

/* ---- LoRa: Ra-02 433 MHz, SF7, BW125, CR4/5, 14 dBm (sync word 0x4C is fixed in the driver) ---- */
static const lm_lora_cfg_t RADIO = { 433000000, 7, 125000, 5, 14 };
static lm_lora_t radio; static lm_link_if_t air; static int radio_ok; static uint32_t t_retry;
static lm_slots_t slots;

/* ---- beacon writes: queued, sent one at a time in the reader window, acked by (id, version) ---- */
#define WR_QUEUE   8
#define WR_RETRIES 3                                   /* after the first send */
#define WR_ACK_MS  (2 * LM_SLOT_PERIOD_MS)
typedef struct { lm_beacon_payload_t p; uint8_t sends; uint32_t sent_at; } wr_t;
static wr_t wq[WR_QUEUE]; static uint8_t wq_head, wq_n;

static void wr_done(uint8_t ok) {
    lm_beacon_ack_t a = { wq[wq_head].p.id, wq[wq_head].p.version, ok };
    uplink_beacon_ack(&a);
    wq_head = (uint8_t)((wq_head + 1) % WR_QUEUE); wq_n--;
}
static void wr_tick(void) {
    if (!wq_n) return;
    wr_t *w = &wq[wq_head];
    if (w->sends && now - w->sent_at < WR_ACK_MS) return;                       /* waiting for the ack */
    if (w->sends > WR_RETRIES) { wr_done(0); return; }                          /* gave up */
    if (!lm_slot_reader_window(&slots, now)) return;
    uint32_t left = (LM_SLOT_READER + 1) * LM_SLOT_MS - lm_slot_phase(&slots, now);
    if (lm_lora_airtime_ms(&RADIO, (uint8_t)(sizeof w->p + 6)) > left) return;             /* would spill into slot 0 */
    lm_beacon_payload_t out = w->p;
    out.age_s = 0;                                                              /* age 0 = a write */
    out.phase_ms = lm_slot_phase(&slots, now);
    lm_send(&air, LM_MSG_BEACON_PAYLOAD, &out, sizeof out);
    w->sends++; w->sent_at = now;
}

static void on_air(uint8_t id, const uint8_t *p, uint8_t n, int8_t rssi, void *ctx) {
    (void)ctx;
    if (id == LM_MSG_BEACON_PAYLOAD && n == sizeof(lm_beacon_payload_t)) {
        lm_beacon_obs_t o; memcpy(&o, p, n); o.rssi = rssi;                    /* wire -> wire: payload is the obs prefix */
        lm_slot_sync(&slots, o.phase_ms, lm_lora_airtime_ms(&RADIO, (uint8_t)(n + 6)), now);
        uplink_beacon_obs(&o);
    } else if (id == LM_MSG_BEACON_ACK && n == sizeof(lm_beacon_ack_t)) {
        lm_beacon_ack_t a; memcpy(&a, p, n);
        const wr_t *w = &wq[wq_head];
        if (wq_n && w->sends && a.id == w->p.id && a.version == w->p.version) wr_done(a.ok);
    }
}

static void radio_init(void) {
    t_retry = now;
    radio_ok = lm_lora_init(&radio, board_lora(), &RADIO) == LM_LORA_OK;
    if (radio_ok) air = lm_lora_as_link(&radio);
}

/* ---- Pi -> STM (uplink.h) ---- */
void app_on_cmd_vel(float v, float w) { motors_set_cmd(v, w, board_millis()); }
void app_on_drop(const lm_drop_cmd_t *m) { dropper_release(m->slot); }
void app_on_link_lost(void) { motors_set_cmd(0.f, 0.f, board_millis()); }
void app_on_beacon_write(const lm_beacon_payload_t *p) {
    if (wq_n == WR_QUEUE) { lm_beacon_ack_t a = { p->id, p->version, 0 }; uplink_beacon_ack(&a); return; }
    wr_t *w = &wq[(wq_head + wq_n++) % WR_QUEUE];
    w->p = *p; w->sends = 0; w->sent_at = 0;
}
int app_on_calibrate(uint8_t target, uint8_t op) {
    switch (op) {
    case LM_CALIB_OP_ENCODER_RESET: odom_reset(&odom); return 1;               /* at B0: W origin = entrance */
    case LM_CALIB_OP_ZERO_BASELINE: (void)target; return 1;                    /* TODO: store R0 of sensor `target` in clean air */
    case LM_CALIB_OP_IMU_BIAS:      return 1;                                   /* TODO: average 2 s of gyro, robot still */
    default:                        return 0;
    }
}

void app_init(void) {
    now = board_millis();
    for (unsigned i = 0; i < N_SENS; i++) sensor_node_init(&sens[i], CFGS[i], now);
    odom_init(&odom, 4000.f, 0.20f);   /* TODO: measured ticks/m, wheel base */
    motors_init(0.20f);
    lm_slots_init(&slots); wq_head = wq_n = 0;
    radio_init();
}
void app_tick(void) {
    now = board_millis(); lm_sensor_det_t d;
    for (unsigned i = 0; i < N_SENS; i++) if (sensor_node_tick(&sens[i], now, &d)) uplink_sensor_det(&d);
    if (now - t_odom >= 20) {                                                   /* 50 Hz */
        lm_wheel_odom_t o; odom_update(&odom, board_enc_left(), board_enc_right(), now, &o);
        uplink_wheel_odom(&o); motors_tick(o.v, o.v, now);
        float acc[3], gyro[3];
        if (board_imu_read(acc, gyro)) {
            lm_imu_raw_t m = { acc[0], acc[1], acc[2], gyro[0], gyro[1], gyro[2], now };
            uplink_imu(&m);
        }
        t_odom = now;
    }
    if (!radio_ok) { if (now - t_retry >= 1000) radio_init(); return; }
    lm_poll(&air, on_air, 0);
    wr_tick();
}
