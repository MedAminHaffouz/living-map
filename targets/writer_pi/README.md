# writer_pi — Writer high-level (Raspberry Pi, ROS 2 Humble/Jazzy)

| Node | Box | Subscribes | Publishes |
|---|---|---|---|
| `micro_ros_agent` | (link to STM, `uros`) | serial `/dev/ttyAMA0` 921600 | bridges the STM node `writer_stm`: `/stm/sensor_det`, `/stm/wheel_odom`, `/stm/imu`, `/beacon/obs` (RSSI), `/beacon/ack` ↑ · `/cmd_vel`, `/stm/drop`, `/beacon/write` ↓ · srv `/stm/calibrate` |
| `stm_adapter` | (STM msgs → standard msgs) | `/stm/wheel_odom`, `/stm/imu` | `/wheel/odom` (nav_msgs/Odometry), `/imu/data_raw` (sensor_msgs/Imu) |
| `rplidar_ros` + `robot_localization` + `slam_toolbox` | SLAM / Localisation | `/scan`, `/wheel/odom`, `/imu/data_raw` | `/map`, TF `map→odom→base_link` |
| `map_processing` (+ Nav2 + explore_lite) | Exploration + Map Processing | `/map` | `/cmd_vel` (via Nav2), RETURN trigger; calls `/stm/calibrate` (ENCODER_RESET, ZERO_BASELINE, IMU_BIAS) once at startup |
| `cv_module` | CV module | camera | `/cv/detections` |
| `event_detection` | Event Detection | `/stm/sensor_det`, `/cv/detections`, TF | `/writer/events` |
| `priority_decision` | Priority decision module | `/writer/events` | `/writer/priority_events` |
| `beacon_dropper` | Beacon Dropper | TF, `/beacon/obs`, `/writer/drop_request` | `/stm/drop`, `/writer/dropped` |
| `beacon_writer` | Beacon Writer | `/writer/priority_events`, `/writer/dropped` | `/beacon/write`, `/writer/drop_request` |

Frame W = `map` (slam_toolbox). Start the robot at the entrance so `map` origin = B0.

```bash
cd ../../ && make ros                    # gen msgs + colcon build
source targets/writer_pi/ros2_ws/install/setup.bash
ros2 launch lm_bringup writer.launch.py
```
Wire msgs in `lm_interfaces/msg/` are generated (`make gen` copies them in) — edit `contracts/schema.yaml`, not the .msg.
`Event`, `PriorityEvent`, `CvDetection` and `srv/Calibrate.srv` are hand-written.
The Writer STM's micro-ROS library must be built with the **same** `lm_interfaces` (see `targets/writer_stm/README.md`).
Needs `micro_ros_agent` (micro-ROS setup for your distro) on the Pi.
