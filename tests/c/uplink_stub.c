/* Host stand-in for targets/writer_stm/app/uros_app.c: records what the app would publish to the Pi. */
#include <string.h>
#include "uplink.h"
int up_det, up_odom, up_imu, up_obs, up_ack;
lm_beacon_obs_t up_last_obs; lm_beacon_ack_t up_last_ack;
void uplink_sensor_det(const lm_sensor_det_t *m) { (void)m; up_det++; }
void uplink_wheel_odom(const lm_wheel_odom_t *m) { (void)m; up_odom++; }
void uplink_imu(const lm_imu_raw_t *m) { (void)m; up_imu++; }
void uplink_beacon_obs(const lm_beacon_obs_t *m) { up_last_obs = *m; up_obs++; }
void uplink_beacon_ack(const lm_beacon_ack_t *m) { up_last_ack = *m; up_ack++; }
