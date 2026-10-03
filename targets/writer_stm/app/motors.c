#include "motors.h"
static float wb, sp_l, sp_r; static uint32_t t_cmd;
void motors_init(float wheel_base_m) { wb = wheel_base_m; }
void motors_set_cmd(float v, float w, uint32_t now) { sp_l = v - 0.5f * w * wb; sp_r = v + 0.5f * w * wb; t_cmd = now; }
void motors_tick(float vl, float vr, uint32_t now) {
    if (now - t_cmd > 300) sp_l = sp_r = 0.f;   /* Pi silent -> stop */
    (void)vl; (void)vr; /* TODO: PID(sp - meas) -> __HAL_TIM_SET_COMPARE(...) */
}
