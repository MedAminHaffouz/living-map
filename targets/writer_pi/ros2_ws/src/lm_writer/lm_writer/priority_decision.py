"""Priority decision module: /writer/events -> /writer/priority_events.
P1: severity*conf bins. P2: LinUCB contextual bandit trained offline in sim (reward from ground truth), frozen on robot.
Arms: IGNORE LOW NORMAL HIGH IMMEDIATE (IMMEDIATE = Beacon Writer rewrites the previous beacon)."""
import rclpy
from rclpy.node import Node
import lm_interfaces.msg as R

def policy(e) -> int:
    s = e.severity * e.conf
    return 4 if s > .8 else 3 if s > .6 else 2 if s > .4 else 1 if s > .2 else 0

class PriorityDecision(Node):
    def __init__(self):
        super().__init__("priority_decision")
        self.pub = self.create_publisher(R.PriorityEvent, "/writer/priority_events", 10)
        self.create_subscription(R.Event, "/writer/events", lambda e: self.pub.publish(R.PriorityEvent(event=e, priority=policy(e))), 10)

def main():
    rclpy.init(); rclpy.spin(PriorityDecision())
