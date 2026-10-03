/* MotorCmd (v, w) -> wheel speed setpoints -> PID -> PWM. Watchdog: stop if no MotorCmd for 300 ms. */
#pragma once
#include <stdint.h>
void motors_init(float wheel_base_m);
void motors_set_cmd(float v, float w, uint32_t now_ms);
void motors_tick(float meas_vl, float meas_vr, uint32_t now_ms);   /* TODO: PID gains, HAL PWM */
