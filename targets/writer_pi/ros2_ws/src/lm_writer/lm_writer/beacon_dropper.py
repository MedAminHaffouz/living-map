"""Beacon Dropper (location-based, not bandit): TF pose + /beacon/obs RSSI of last beacon
-> /stm/drop (servo release) + /writer/dropped (which beacon id is now at which pose).
Drop when: B0 at entry | dist to last > d_max | RSSI(last) < rssi_min | event requested by beacon_writer."""
import math, rclpy
from rclpy.node import Node
from rclpy.time import Time
from tf2_ros import Buffer, TransformListener
from geometry_msgs.msg import PointStamped
from std_msgs.msg import UInt8
import lm_interfaces.msg as R

class BeaconDropper(Node):
    def __init__(self):
        super().__init__("beacon_dropper")
        self.d_max = self.declare_parameter("d_max_m", 4.0).value
        self.rssi_min = self.declare_parameter("rssi_min_dbm", -80).value
        self.n_beacons = self.declare_parameter("n_beacons", 10).value
        self.tf = Buffer(); TransformListener(self.tf, self)
        self.p_drop = self.create_publisher(R.DropCmd, "/stm/drop", 10)
        self.p_dropped = self.create_publisher(PointStamped, "/writer/dropped", 10)  # point.z carries beacon id
        self.create_subscription(R.BeaconObs, "/beacon/obs", self.on_obs, 20)
        self.create_subscription(UInt8, "/writer/drop_request", lambda _: self.drop("event"), 10)
        self.next_id, self.last, self.last_rssi = 0, None, 0
        self.create_timer(0.2, self.tick)
    def pose(self):
        try:
            t = self.tf.lookup_transform("map", "base_link", Time()).transform.translation; return t.x, t.y
        except Exception: return None
    def on_obs(self, o):
        if self.next_id and o.id == self.next_id - 1: self.last_rssi = o.rssi
    def drop(self, reason):
        p = self.pose()
        if p is None or self.next_id >= self.n_beacons: return
        self.p_drop.publish(R.DropCmd(slot=self.next_id, beacon_id=self.next_id))
        m = PointStamped(); m.header.stamp = self.get_clock().now().to_msg(); m.header.frame_id = "map"
        m.point.x, m.point.y, m.point.z = p[0], p[1], float(self.next_id); self.p_dropped.publish(m)
        self.get_logger().info(f"drop B{self.next_id} ({reason}) at {p}"); self.last = p; self.next_id += 1
    def tick(self):
        p = self.pose()
        if p is None: return
        if self.last is None: self.drop("entry B0")
        elif math.hypot(p[0] - self.last[0], p[1] - self.last[1]) > self.d_max: self.drop("spacing")
        elif self.last_rssi and self.last_rssi < self.rssi_min: self.drop("rssi")

def main():
    rclpy.init(); rclpy.spin(BeaconDropper())
