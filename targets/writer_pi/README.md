# writer_pi — Writer high-level (Raspberry Pi, ROS 2 Humble/Jazzy)

| Node | Box | Subscribes | Publishes |
|---|---|---|---|
| `stm_bridge` | (link to STM) | `/cmd_vel`, `/stm/calib`, `/stm/drop` | `/stm/sensor_det`, `/wheel/odom`, `/imu/data_raw` |
| `radio_bridge` | (link to radio ESP) | `/beacon/write` | `/beacon/obs` (RSSI), `/beacon/ack` |
| `rplidar_ros` + `robot_localization` + `slam_toolbox` | SLAM / Localisation | `/scan`, `/wheel/odom`, `/imu/data_raw` | `/map`, TF `map→odom→base_link` |
| `map_processing` (+ Nav2 + explore_lite) | Exploration + Map Processing | `/map` | `/cmd_vel` (via Nav2), RETURN trigger |
| `cv_module` | CV module | camera | `/cv/detections` |
| `event_detection` | Event Detection | `/stm/sensor_det`, `/cv/detections`, TF | `/writer/events` |
| `priority_decision` | Priority decision module | `/writer/events` | `/writer/priority_events` |
| `beacon_dropper` | Beacon Dropper | TF, `/beacon/obs`, `/writer/drop_request` | `/stm/drop`, `/writer/dropped` |
| `beacon_writer` | Beacon Writer | `/writer/priority_events`, `/writer/dropped` | `/beacon/write`, `/writer/drop_request` |

Frame W = `map` (slam_toolbox). Start the robot at the entrance so `map` origin = B0.

```bash
cd ../../ && make ros                    # gen msgs + colcon build
source targets/writer_pi/ros2_ws/install/setup.bash
export LM_REPO=$(pwd)                    # nodes import libs/ + generated contracts from here
ros2 launch lm_bringup writer.launch.py
```
Wire msgs in `lm_interfaces/msg/` are generated (`make gen`) — edit `contracts/schema.yaml`, not the .msg.
`Event`, `PriorityEvent`, `CvDetection` are Pi-internal and hand-written.
