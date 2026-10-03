/* Encoders -> differential-drive odometry, integrated on the STM (exact timing), sent as WheelOdom ~50 Hz. */
#pragma once
#include <stdint.h>
#include "lm_msgs.h"
typedef struct { float ticks_per_m, wheel_base_m; int32_t last_l, last_r; float x, y, th, v, w; uint32_t t_last; } odom_t;
void odom_init(odom_t *o, float ticks_per_m, float wheel_base_m);
void odom_update(odom_t *o, int32_t ticks_l, int32_t ticks_r, uint32_t now_ms, lm_wheel_odom_t *out);
void odom_reset(odom_t *o);   /* CalibCmd ENCODER_RESET: called at B0 so W origin = entrance */
