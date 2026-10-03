"""Exploration + Map Processing: owns the Writer's mission flow on top of Nav2 + explore_lite.
- watches /map coverage, battery, beacons left, time budget
- triggers RETURN (pause explore_lite, NavigateToPose to B0 at (0,0) in map)
- at the entrance: exports mission log (events + beacon poses + map) for the ONA upload path (if used)
TODO(owner): budget logic, explore_lite resume/pause, Nav2 action client."""
import rclpy
from rclpy.node import Node
from nav_msgs.msg import OccupancyGrid

class MapProcessing(Node):
    def __init__(self):
        super().__init__("map_processing")
        self.budget_s = self.declare_parameter("explore_budget_s", 300.0).value
        self.create_subscription(OccupancyGrid, "/map", self.on_map, 1)
        self.t0 = self.get_clock().now()
    def on_map(self, m):
        elapsed = (self.get_clock().now() - self.t0).nanoseconds / 1e9
        if elapsed > self.budget_s: self.get_logger().info("budget reached -> RETURN (TODO)")

def main():
    rclpy.init(); rclpy.spin(MapProcessing())
