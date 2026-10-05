"""Writer Pi bringup: micro-ROS agent (STM link) -> drivers -> localization -> SLAM -> exploration -> Living Map nodes.
Nav2 + explore_lite are launched separately until tuned (see README)."""
from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory as share
import os
CFG = os.path.join(share("lm_bringup"), "config")

def lm(name, **p): return Node(package="lm_writer", executable=name, name=name, parameters=[os.path.join(CFG, "writer.yaml"), p])

def generate_launch_description():
    return LaunchDescription([
        Node(package="rplidar_ros", executable="rplidar_node", parameters=[{"serial_port": "/dev/ttyUSB1", "frame_id": "laser"}]),
        Node(package="tf2_ros", executable="static_transform_publisher", arguments=["0.1", "0", "0.15", "0", "0", "0", "base_link", "laser"]),
        Node(package="robot_localization", executable="ekf_node", parameters=[os.path.join(CFG, "ekf.yaml")]),
        Node(package="slam_toolbox", executable="async_slam_toolbox_node", parameters=[os.path.join(CFG, "slam_toolbox.yaml")]),
        Node(package="micro_ros_agent", executable="micro_ros_agent", name="micro_ros_agent",
             arguments=["serial", "--dev", "/dev/ttyAMA0", "-b", "921600"]),          # Writer STM (uros link)
        lm("stm_adapter"), lm("cv_module"), lm("event_detection"),
        lm("priority_decision"), lm("beacon_dropper"), lm("beacon_writer"), lm("map_processing"),
    ])
