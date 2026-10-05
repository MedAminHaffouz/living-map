#include "calib.h"
#include "app.h"
void calib_handle(uint8_t target, uint8_t op) {
    switch (op) {
    case LM_CALIB_OP_ZERO_BASELINE: app_sensor_baseline(target); break;
    case LM_CALIB_OP_IMU_BIAS:      /* TODO: average 2 s of gyro */ break;
    case LM_CALIB_OP_ENCODER_RESET: app_odom_reset(); break;
    }
}
