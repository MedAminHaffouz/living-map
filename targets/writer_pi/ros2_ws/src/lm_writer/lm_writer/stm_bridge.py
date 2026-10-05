"""STM <-> Pi bridge. The ONLY node that touches the STM UART.
Up:   SensorDet -> /stm/sensor_det ; WheelOdom -> /wheel/odom (nav_msgs/Odometry) ; ImuRaw -> /imu/data_raw
Down: /cmd_vel -> MotorCmd ; /stm/drop -> DropCmd
Stamps every upstream msg with Pi time on receipt (STM t_ms kept in the payload for latency checks)."""
from . import _paths  # noqa
import math, serial, rclpy
from rclpy.node import Node
from nav_msgs.msg import Odometry
from sensor_msgs.msg import Imu
from geometry_msgs.msg import Twist
import lm_interfaces.msg as R
import lm_msgs as M
from lm_core.link import Decoder, encode
from ._conv import to_ros, from_ros

class StmBridge(Node):
    def __init__(self):
        super().__init__("stm_bridge")
        port = self.declare_parameter("port", "/dev/ttyAMA0").value
        self.ser = serial.Serial(port, self.declare_parameter("baud", 921600).value, timeout=0)
        self.dec = Decoder()
        self.p_det = self.create_publisher(R.SensorDet, "/stm/sensor_det", 20)
        self.p_odom = self.create_publisher(Odometry, "/wheel/odom", 20)
        self.p_imu = self.create_publisher(Imu, "/imu/data_raw", 50)
        self.create_subscription(Twist, "/cmd_vel", self.on_cmd_vel, 10)
        self.create_subscription(R.DropCmd, "/stm/drop", lambda m: self.send(from_ros(m, M.DropCmd)), 10)
        self.create_timer(0.002, self.poll)

    def send(self, dc): self.ser.write(encode(dc.ID, dc.pack()))
    def on_cmd_vel(self, t): self.send(M.MotorCmd(v=t.linear.x, w=t.angular.z, t_ms=0))

    def poll(self):
        for mid, payload in self.dec.feed(self.ser.read(512)):
            T = M.BY_ID.get(mid)
            if T is None or len(payload) != T.SIZE: continue
            d = T.unpack(payload)
            if T is M.SensorDet: self.p_det.publish(to_ros(d, R.SensorDet))
            elif T is M.WheelOdom: self.p_odom.publish(self.odom(d))
            elif T is M.ImuRaw: self.p_imu.publish(self.imu(d))

    def odom(self, d):
        o = Odometry(); o.header.stamp = self.get_clock().now().to_msg()
        o.header.frame_id, o.child_frame_id = "odom", "base_link"
        o.pose.pose.position.x, o.pose.pose.position.y = d.x, d.y
        o.pose.pose.orientation.z, o.pose.pose.orientation.w = math.sin(d.theta / 2), math.cos(d.theta / 2)
        o.twist.twist.linear.x, o.twist.twist.angular.z = d.v, d.w
        return o   # TODO: fill covariances from encoder calibration

    def imu(self, d):
        m = Imu(); m.header.stamp = self.get_clock().now().to_msg(); m.header.frame_id = "imu_link"
        m.linear_acceleration.x, m.linear_acceleration.y, m.linear_acceleration.z = d.ax, d.ay, d.az
        m.angular_velocity.x, m.angular_velocity.y, m.angular_velocity.z = d.gx, d.gy, d.gz
        return m

def main():
    rclpy.init(); rclpy.spin(StmBridge())
