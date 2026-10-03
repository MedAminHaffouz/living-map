#include "app.h"
#include "lm_link.h"
#include "lm_msgs.h"
#include "sensor_node.h"
#include "odometry.h"
#include "motors.h"
#include "calib.h"
#include "dropper.h"

/* sensor nodes = config only (see sensor_node.h) */
static const sensor_cfg_t CFG_GAS  = { LM_EVENT_TYPE_GAS,  board_read_gas_ppm,     200.f, 120.f, 3, 5, 60000, 100 };
static const sensor_cfg_t CFG_TEMP = { LM_EVENT_TYPE_FIRE, board_read_temp_c,      55.f,  45.f,  3, 5, 0,     100 };
static const sensor_cfg_t CFG_SMOKE= { LM_EVENT_TYPE_SMOKE,board_read_smoke_ratio, 1.8f,  1.4f,  3, 5, 20000, 100 };
static const sensor_cfg_t *CFGS[] = { &CFG_GAS, &CFG_TEMP, &CFG_SMOKE };
#define N_SENS (sizeof CFGS / sizeof *CFGS)
static sensor_node_t sens[N_SENS];
static odom_t odom; static lm_decoder_t dec; static uint32_t t_odom, t_hb;

static void send(uint8_t id, const void *p, uint8_t n) { uint8_t f[LM_MAX_PAYLOAD + 6]; board_uart_write(f, lm_encode(id, p, n, f)); }

static void on_msg(uint8_t id, const uint8_t *p, uint8_t n, void *ctx) {
    (void)ctx;
    switch (id) {
    case LM_MSG_MOTOR_CMD: if (n == sizeof(lm_motor_cmd_t)) { const lm_motor_cmd_t *m = (const void *)p; motors_set_cmd(m->v, m->w, board_millis()); } break;
    case LM_MSG_CALIB_CMD: if (n == sizeof(lm_calib_cmd_t)) calib_handle((const void *)p); break;
    case LM_MSG_DROP_CMD:  if (n == sizeof(lm_drop_cmd_t))  dropper_release(((const lm_drop_cmd_t *)p)->slot); break;
    }
}
void app_uart_rx_byte(uint8_t b) { lm_decoder_feed(&dec, b, on_msg, 0); }
void app_sensor_baseline(uint8_t target) { (void)target; /* TODO: store R0 for MQ sensors */ }
void app_odom_reset(void) { odom_reset(&odom); }

void app_init(void) {
    uint32_t now = board_millis();
    for (unsigned i = 0; i < N_SENS; i++) sensor_node_init(&sens[i], CFGS[i], now);
    odom_init(&odom, 4000.f, 0.20f);   /* TODO: measured ticks/m, wheel base */
    motors_init(0.20f); lm_decoder_init(&dec);
}
void app_tick(void) {
    uint32_t now = board_millis(); lm_sensor_det_t d;
    for (unsigned i = 0; i < N_SENS; i++) if (sensor_node_tick(&sens[i], now, &d)) send(LM_MSG_SENSOR_DET, &d, sizeof d);
    if (now - t_odom >= 20) { lm_wheel_odom_t o; odom_update(&odom, board_enc_left(), board_enc_right(), now, &o);
                              send(LM_MSG_WHEEL_ODOM, &o, sizeof o); motors_tick(o.v, o.v, now); t_odom = now; }
    if (now - t_hb >= 1000) { lm_heartbeat_t h = { LM_NODE_ID_WRITER, 0, now }; send(LM_MSG_HEARTBEAT, &h, sizeof h); t_hb = now; }
}
