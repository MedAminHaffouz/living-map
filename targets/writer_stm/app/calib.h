/* Recalibration, called by the micro-ROS calib service: ZERO_BASELINE (MQ R0 in clean air at entrance),
   IMU_BIAS (robot still), ENCODER_RESET (at B0). */
#pragma once
#include "lm_msgs.h"
void calib_handle(uint8_t target, uint8_t op);
