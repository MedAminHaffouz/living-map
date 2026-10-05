/* micro-ROS side of the Writer STM (uros_app.c). No ROS types here: main.c includes this, not rcl.
   main.c must set up the micro-ROS transport (rmw_uros_set_custom_transport on the Pi UART) before uros_app_init(). */
#pragma once
void uros_app_init(void);
void uros_app_tick(void);   /* call every loop, after app_tick(): agent state machine + rclc executor spin */
