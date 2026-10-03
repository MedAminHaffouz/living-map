#include "calib.h"
#include "app.h"
void calib_handle(const lm_calib_cmd_t *c) {
    switch (c->op) {
    case LM_CALIB_OP_ZERO_BASELINE: app_sensor_baseline(c->target); break;
    case LM_CALIB_OP_IMU_BIAS:      /* TODO: average 2 s of gyro */ break;
    case LM_CALIB_OP_ENCODER_RESET: app_odom_reset(); break;
    }
}
