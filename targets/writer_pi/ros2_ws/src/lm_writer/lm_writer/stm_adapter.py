"""STM adapter: the Writer STM publishes compact lm_interfaces msgs over micro-ROS; this turns them into the
standard types robot_localization / slam_toolbox expect. Stamped with Pi time on receipt (STM t_ms kept upstream).
  /stm/wheel_odom (WheelOdom) -> /wheel/odom   nav_msgs/Odometry  (covariances from odom_var_xy, odom_var_theta)
  /stm/imu        (ImuRaw)    -> /imu/data_raw sensor_msgs/Imu    (no orientation: orientation_covariance[0] = -1)"""
import math, rclpy
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from nav_msgs.msg import Odometry
from sensor_msgs.msg import Imu
import lm_interfaces.msg as R

UNUSED = 1e6   # variance for the 3D axes a 2D diff-drive can't observe (z, roll, pitch)

def diag6(var_xy, var_theta):
    c = [0.0] * 36
    for i, v in enumerate((var_xy, var_xy, UNUSED, UNUSED, UNUSED, var_theta)): c[7 * i] = v
    return c

class StmAdapter(Node):
    def __init__(self):
        super().__init__("stm_adapter")
        self.cov = diag6(self.declare_parameter("odom_var_xy", 0.01).value,
                         self.declare_parameter("odom_var_theta", 0.03).value)
        self.p_odom = self.create_publisher(Odometry, "/wheel/odom", 20)
        self.p_imu = self.create_publisher(Imu, "/imu/data_raw", 50)
        self.create_subscription(R.WheelOdom, "/stm/wheel_odom", self.on_odom, qos_profile_sensor_data)   # STM: best effort
        self.create_subscription(R.ImuRaw, "/stm/imu", self.on_imu, qos_profile_sensor_data)

    def on_odom(self, d):
        o = Odometry(); o.header.stamp = self.get_clock().now().to_msg()
        o.header.frame_id, o.child_frame_id = "odom", "base_link"
        o.pose.pose.position.x, o.pose.pose.position.y = float(d.x), float(d.y)
        o.pose.pose.orientation.z, o.pose.pose.orientation.w = math.sin(d.theta / 2), math.cos(d.theta / 2)
        o.twist.twist.linear.x, o.twist.twist.angular.z = float(d.v), float(d.w)
        o.pose.covariance = self.cov; o.twist.covariance = self.cov
        self.p_odom.publish(o)

    def on_imu(self, d):
        m = Imu(); m.header.stamp = self.get_clock().now().to_msg(); m.header.frame_id = "imu_link"
        m.orientation_covariance[0] = -1.0                       # REP 145: orientation not provided
        m.linear_acceleration.x, m.linear_acceleration.y, m.linear_acceleration.z = float(d.ax), float(d.ay), float(d.az)
        m.angular_velocity.x, m.angular_velocity.y, m.angular_velocity.z = float(d.gx), float(d.gy), float(d.gz)
        self.p_imu.publish(m)                                    # TODO: accel / gyro covariances from the IMU datasheet

def main():
    rclpy.init(); rclpy.spin(StmAdapter())
