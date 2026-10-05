"""Exploration + Map Processing: owns the Writer's mission flow on top of Nav2 + explore_lite.
- watches /map coverage, battery, beacons left, time budget
- triggers RETURN (pause explore_lite, NavigateToPose to B0 at (0,0) in map)
- at the entrance: exports mission log (events + beacon poses + map) for the ONA upload path (if used)
- at startup (robot still, at the entrance): calibrates the STM once via /stm/calibrate:
  ENCODER_RESET (W origin = B0), ZERO_BASELINE (gas/smoke R0 in clean air), IMU_BIAS
TODO(owner): budget logic, explore_lite resume/pause, Nav2 action client."""
import rclpy
from rclpy.node import Node
from nav_msgs.msg import OccupancyGrid
from lm_interfaces.srv import Calibrate

CALIB_OPS = [3, 1, 2]   # CalibOp: ENCODER_RESET, ZERO_BASELINE, IMU_BIAS (contracts/schema.yaml)

class MapProcessing(Node):
    def __init__(self):
        super().__init__("map_processing")
        self.budget_s = self.declare_parameter("explore_budget_s", 300.0).value
        self.create_subscription(OccupancyGrid, "/map", self.on_map, 1)
        self.t0 = self.get_clock().now()
        self.calib = self.create_client(Calibrate, "/stm/calibrate")
        self.calib_todo = list(CALIB_OPS); self.calib_busy = False
        self.calib_timer = self.create_timer(0.5, self.calibrate_step)   # waits for the STM to come up

    def calibrate_step(self):
        """One op at a time, once each, as soon as the service is there."""
        if self.calib_busy or not self.calib.service_is_ready(): return
        if not self.calib_todo: self.calib_timer.cancel(); return
        op = self.calib_todo.pop(0); self.calib_busy = True
        self.calib.call_async(Calibrate.Request(target=0, op=op)).add_done_callback(lambda f, op=op: self.calibrated(op, f))

    def calibrated(self, op, fut):
        self.calib_busy = False
        ok = fut.result() is not None and fut.result().ok
        (self.get_logger().info if ok else self.get_logger().error)(f"calibrate op {op}: {'ok' if ok else 'FAILED'}")

    def on_map(self, m):
        elapsed = (self.get_clock().now() - self.t0).nanoseconds / 1e9
        if elapsed > self.budget_s: self.get_logger().info("budget reached -> RETURN (TODO)")

def main():
    rclpy.init(); rclpy.spin(MapProcessing())
