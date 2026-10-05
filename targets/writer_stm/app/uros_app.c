/* micro-ROS transport for the Writer STM: the ONLY file that includes ROS headers.
   Node "writer_stm". Pubs: /stm/sensor_det, /stm/wheel_odom, /stm/imu (best effort), /beacon/obs, /beacon/ack (reliable).
   Subs: /cmd_vel, /stm/drop, /beacon/write. Service: /stm/calibrate.
   Agent state machine WAIT_AGENT -> CONNECTED -> LOST (-> WAIT_AGENT), driven by rmw_uros_ping_agent.
   Wire <-> ROS conversion only through the generated lm_uros_conv.h (field by field, never memcpy). */
#include <string.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <rmw_microros/rmw_microros.h>
#include <geometry_msgs/msg/twist.h>
#include <lm_interfaces/msg/sensor_det.h>
#include <lm_interfaces/msg/wheel_odom.h>
#include <lm_interfaces/msg/imu_raw.h>
#include <lm_interfaces/msg/beacon_obs.h>
#include <lm_interfaces/msg/beacon_ack.h>
#include <lm_interfaces/msg/beacon_payload.h>
#include <lm_interfaces/msg/drop_cmd.h>
#include <lm_interfaces/srv/calibrate.h>
#include "lm_uros_conv.h"
#include "app.h"
#include "uplink.h"
#include "uros_app.h"

#define PING_WAIT_MS      500    /* WAIT_AGENT: how often to look for the agent */
#define PING_ALIVE_MS     1000   /* CONNECTED: liveness check period */
#define PING_TIMEOUT_MS   50
#define PING_ATTEMPTS     3
#define N_HANDLES         4      /* 3 subscriptions + 1 service */

typedef enum { WAIT_AGENT, CONNECTED, LOST } uros_state_t;
static uros_state_t st = WAIT_AGENT;
static uint32_t t_ping;

static rcl_allocator_t alloc; static rclc_support_t support; static rcl_node_t node; static rclc_executor_t exec;
static rcl_publisher_t p_det, p_odom, p_imu, p_obs, p_ack;
static rcl_subscription_t s_cmd, s_drop, s_write;
static rcl_service_t srv_calib;

static geometry_msgs__msg__Twist m_cmd;
static lm_interfaces__msg__DropCmd m_drop;
static lm_interfaces__msg__BeaconPayload m_write;
static lm_interfaces__srv__Calibrate_Request calib_req;
static lm_interfaces__srv__Calibrate_Response calib_res;

/* ---- Pi -> STM ---- */
static void on_cmd_vel(const void *msg) { const geometry_msgs__msg__Twist *t = msg; app_on_cmd_vel((float)t->linear.x, (float)t->angular.z); }
static void on_drop(const void *msg) { lm_drop_cmd_t d; lm_drop_cmd_from_ros(&d, msg); app_on_drop(&d); }
static void on_write(const void *msg) { lm_beacon_payload_t p; lm_beacon_payload_from_ros(&p, msg); app_on_beacon_write(&p); }
static void on_calibrate(const void *req, void *res) {
    const lm_interfaces__srv__Calibrate_Request *rq = req;
    ((lm_interfaces__srv__Calibrate_Response *)res)->ok = app_on_calibrate(rq->target, rq->op) != 0;
}

/* ---- STM -> Pi (uplink.h): dropped while not connected ---- */
#define UPLINK(fn, wire_t, ros_t, conv, pub)                                \
    void fn(const wire_t *m) {                                             \
        if (st != CONNECTED) return;                                       \
        ros_t r; conv(&r, m); (void)rcl_publish(&pub, &r, NULL);           \
    }
UPLINK(uplink_sensor_det, lm_sensor_det_t,  lm_interfaces__msg__SensorDet, lm_ros_from_sensor_det, p_det)
UPLINK(uplink_wheel_odom, lm_wheel_odom_t,  lm_interfaces__msg__WheelOdom, lm_ros_from_wheel_odom, p_odom)
UPLINK(uplink_imu,        lm_imu_raw_t,     lm_interfaces__msg__ImuRaw,    lm_ros_from_imu_raw,    p_imu)
UPLINK(uplink_beacon_obs, lm_beacon_obs_t,  lm_interfaces__msg__BeaconObs, lm_ros_from_beacon_obs, p_obs)
UPLINK(uplink_beacon_ack, lm_beacon_ack_t,  lm_interfaces__msg__BeaconAck, lm_ros_from_beacon_ack, p_ack)

/* ---- entities ---- */
#define MSG(pkg, name) ROSIDL_GET_MSG_TYPE_SUPPORT(pkg, msg, name)
#define TRY(x) do { if ((x) != RCL_RET_OK) return 0; } while (0)
static void zero_entities(void) {   /* so destroy_entities() is safe on a half-built set */
    memset(&support, 0, sizeof support);
    node = rcl_get_zero_initialized_node();
    p_det = p_odom = p_imu = p_obs = p_ack = rcl_get_zero_initialized_publisher();
    s_cmd = s_drop = s_write = rcl_get_zero_initialized_subscription();
    srv_calib = rcl_get_zero_initialized_service();
    exec = rclc_executor_get_zero_initialized_executor();
}
static int create_entities(void) {
    zero_entities();
    alloc = rcl_get_default_allocator();
    TRY(rclc_support_init(&support, 0, NULL, &alloc));
    TRY(rclc_node_init_default(&node, "writer_stm", "", &support));
    TRY(rclc_publisher_init_best_effort(&p_det,  &node, MSG(lm_interfaces, SensorDet), "/stm/sensor_det"));
    TRY(rclc_publisher_init_best_effort(&p_odom, &node, MSG(lm_interfaces, WheelOdom), "/stm/wheel_odom"));
    TRY(rclc_publisher_init_best_effort(&p_imu,  &node, MSG(lm_interfaces, ImuRaw),    "/stm/imu"));
    TRY(rclc_publisher_init_default(&p_obs, &node, MSG(lm_interfaces, BeaconObs), "/beacon/obs"));
    TRY(rclc_publisher_init_default(&p_ack, &node, MSG(lm_interfaces, BeaconAck), "/beacon/ack"));
    TRY(rclc_subscription_init_default(&s_cmd,   &node, MSG(geometry_msgs, Twist),        "/cmd_vel"));
    TRY(rclc_subscription_init_default(&s_drop,  &node, MSG(lm_interfaces, DropCmd),      "/stm/drop"));
    TRY(rclc_subscription_init_default(&s_write, &node, MSG(lm_interfaces, BeaconPayload), "/beacon/write"));
    TRY(rclc_service_init_default(&srv_calib, &node, ROSIDL_GET_SRV_TYPE_SUPPORT(lm_interfaces, srv, Calibrate), "/stm/calibrate"));
    TRY(rclc_executor_init(&exec, &support.context, N_HANDLES, &alloc));
    TRY(rclc_executor_add_subscription(&exec, &s_cmd,   &m_cmd,   on_cmd_vel, ON_NEW_DATA));
    TRY(rclc_executor_add_subscription(&exec, &s_drop,  &m_drop,  on_drop,    ON_NEW_DATA));
    TRY(rclc_executor_add_subscription(&exec, &s_write, &m_write, on_write,   ON_NEW_DATA));
    TRY(rclc_executor_add_service(&exec, &srv_calib, &calib_req, &calib_res, on_calibrate));
    return 1;
}
static void destroy_entities(void) {
    rmw_context_t *ctx = rcl_context_get_rmw_context(&support.context);
    if (ctx) (void)rmw_uros_set_context_entity_destroy_session_timeout(ctx, 0);   /* agent is gone: don't wait for replies */
    (void)rcl_publisher_fini(&p_det, &node);  (void)rcl_publisher_fini(&p_odom, &node); (void)rcl_publisher_fini(&p_imu, &node);
    (void)rcl_publisher_fini(&p_obs, &node);  (void)rcl_publisher_fini(&p_ack, &node);
    (void)rcl_subscription_fini(&s_cmd, &node); (void)rcl_subscription_fini(&s_drop, &node); (void)rcl_subscription_fini(&s_write, &node);
    (void)rcl_service_fini(&srv_calib, &node);
    (void)rclc_executor_fini(&exec);
    (void)rcl_node_fini(&node);
    (void)rclc_support_fini(&support);
}

void uros_app_init(void) { zero_entities(); st = WAIT_AGENT; t_ping = board_millis(); }

void uros_app_tick(void) {
    uint32_t now = board_millis();
    switch (st) {
    case WAIT_AGENT:
        if (now - t_ping < PING_WAIT_MS) break;
        t_ping = now;
        if (rmw_uros_ping_agent(PING_TIMEOUT_MS, 1) == RMW_RET_OK) {
            if (create_entities()) st = CONNECTED;
            else destroy_entities();                                      /* half-built: tear down, try again */
        }
        break;
    case CONNECTED:
        if (now - t_ping >= PING_ALIVE_MS) {
            t_ping = now;
            if (rmw_uros_ping_agent(PING_TIMEOUT_MS, PING_ATTEMPTS) != RMW_RET_OK) { st = LOST; break; }
        }
        (void)rclc_executor_spin_some(&exec, RCL_MS_TO_NS(1));
        break;
    case LOST:
        app_on_link_lost();
        destroy_entities();
        st = WAIT_AGENT; t_ping = now;
        break;
    }
}
