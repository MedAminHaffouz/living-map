"""Event Detection: /stm/sensor_det + /cv/detections + TF(map->base_link) [+ /scan for CV range]
-> pose-tagged, deduplicated /writer/events (frame W = map, origin B0).
Rules (P1): one event per (type, radius r); co-occurrence raises severity; smoke lowers CV conf.
Sensor-level debounce already happened on the STM — do NOT re-debounce here."""
import math, rclpy
from rclpy.node import Node
from rclpy.time import Time
from tf2_ros import Buffer, TransformListener
import lm_interfaces.msg as R

class EventDetection(Node):
    def __init__(self):
        super().__init__("event_detection")
        self.r = self.declare_parameter("dedupe_radius_m", 2.0).value
        self.tf = Buffer(); TransformListener(self.tf, self)
        self.pub = self.create_publisher(R.Event, "/writer/events", 10)
        self.create_subscription(R.SensorDet, "/stm/sensor_det", self.on_det, 20)
        self.create_subscription(R.CvDetection, "/cv/detections", self.on_cv, 10)
        self.events, self.nid = [], 0
    def pose(self):
        try:
            t = self.tf.lookup_transform("map", "base_link", Time()).transform.translation; return t.x, t.y
        except Exception: return None
    def emit(self, typ, sev, conf, x, y):
        if any(e.type == typ and math.hypot(e.x - x, e.y - y) < self.r for e in self.events): return
        self.nid += 1
        e = R.Event(stamp=self.get_clock().now().to_msg(), id=self.nid, type=typ, severity=sev, conf=conf, x=x, y=y)
        self.events.append(e); self.pub.publish(e)
    def on_det(self, d):
        p = self.pose()
        if d.detected and p: self.emit(d.type, min(1.0, d.value / 100.0), d.conf / 255.0, *p)  # TODO per-type severity map
    def on_cv(self, c):
        p = self.pose()
        if p: self.emit(3, {3: 0.9, 2: 0.6}.get(c.posture, 0.4), c.conf, *p)  # TODO: project with bearing + /scan range

def main():
    rclpy.init(); rclpy.spin(EventDetection())
