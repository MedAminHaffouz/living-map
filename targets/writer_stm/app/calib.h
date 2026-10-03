/* Recalibration on CalibCmd: ZERO_BASELINE (MQ R0 in clean air at entrance), IMU_BIAS (robot still), ENCODER_RESET (at B0). */
#pragma once
#include "lm_msgs.h"
void calib_handle(const lm_calib_cmd_t *c);
