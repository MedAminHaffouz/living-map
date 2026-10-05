Declaration-only stand-ins for the rcl / rclc / rmw_microros / rosidl C API (signatures as in ROS 2 Humble/Jazzy
micro-ROS), so `targets/writer_stm/app/uros_app.c` gets a `-fsyntax-only` check on a host without micro-ROS.
Nothing here links or runs. `lm_interfaces/msg/*.h` are generated per test run from `contracts/generated/ros/msg`.
