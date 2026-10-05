/* The seam between Writer STM logic (app.c) and its transport to the Pi (uros_app.c, micro-ROS over UART).
   app.c never sees ROS types; uros_app.c never sees sensors, motors or the radio.
   Host tests replace uros_app.c with tests/c/uplink_stub.c. */
#pragma once
#include <stdint.h>
#include "lm_msgs.h"

/* ---- STM -> Pi: implemented by uros_app.c (dropped silently while the agent is not connected) ---- */
void uplink_sensor_det(const lm_sensor_det_t *m);     /* /stm/sensor_det  best effort */
void uplink_wheel_odom(const lm_wheel_odom_t *m);     /* /stm/wheel_odom  best effort */
void uplink_imu(const lm_imu_raw_t *m);               /* /stm/imu         best effort */
void uplink_beacon_obs(const lm_beacon_obs_t *m);     /* /beacon/obs      reliable */
void uplink_beacon_ack(const lm_beacon_ack_t *m);     /* /beacon/ack      reliable: result of every /beacon/write */

/* ---- Pi -> STM: implemented by app.c, called by uros_app.c ---- */
void app_on_cmd_vel(float v, float w);                      /* /cmd_vel linear.x, angular.z */
void app_on_drop(const lm_drop_cmd_t *m);                   /* /stm/drop */
void app_on_beacon_write(const lm_beacon_payload_t *p);    /* /beacon/write: queued, sent over LoRa, acked via uplink_beacon_ack */
int  app_on_calibrate(uint8_t target, uint8_t op);          /* /stm/calibrate service, op = lm_calib_op_t; 1 = ok */
void app_on_link_lost(void);                                /* agent gone: stop motors */
