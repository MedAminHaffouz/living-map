"""Beacon Writer: /writer/priority_events + /writer/dropped -> /beacon/write (BeaconPayload, sent over LoRa by the Writer STM).
- every dropped beacon gets a payload (JUNCTION if no event): WHERE = (dir, dist) to previous beacon
- event with priority > IGNORE near no beacon -> /writer/drop_request, payload attached to the new beacon
- IMMEDIATE -> rewrite previous beacon (version+1) with the event
- the STM retries each write until the beacon acks; /beacon/ack carries the outcome (ok=0 after 3 retries).
  TODO: re-queue or re-drop on ok=0."""
import math, rclpy
from rclpy.node import Node
from geometry_msgs.msg import PointStamped
from std_msgs.msg import UInt8
import lm_interfaces.msg as R

class BeaconWriter(Node):
    def __init__(self):
        super().__init__("beacon_writer")
        self.pub = self.create_publisher(R.BeaconPayload, "/beacon/write", 10)
        self.p_req = self.create_publisher(UInt8, "/writer/drop_request", 10)
        self.create_subscription(R.PriorityEvent, "/writer/priority_events", self.on_event, 10)
        self.create_subscription(PointStamped, "/writer/dropped", self.on_dropped, 10)
        self.beacons = {}       # id -> (x, y, payload)
        self.pending = []       # events waiting for a fresh beacon
    def payload(self, bid, x, y, ev=None, prio=1, version=0):
        prev = self.beacons.get(bid - 1)
        dx, dy = (prev[0] - x, prev[1] - y) if prev else (0.0, 0.0)
        return R.BeaconPayload(id=bid, what=ev.type if ev else 4, prio=prio, conf=int(255 * (ev.conf if ev else 1.0)),
                               dir_deg=int(math.degrees(math.atan2(dy, dx)) % 360), dist_cm=int(100 * math.hypot(dx, dy)),
                               prev_id=max(bid - 1, 0), next_id=255, age_s=0, version=version, flags=0)
    def on_dropped(self, m):
        bid, x, y = int(m.point.z), m.point.x, m.point.y
        ev, prio = self.pending.pop(0) if self.pending else (None, 1)
        pl = self.payload(bid, x, y, ev, prio); self.beacons[bid] = (x, y, pl); self.pub.publish(pl)
        if bid - 1 in self.beacons:     # link previous beacon forward: "next beacons pointer"
            px, py, ppl = self.beacons[bid - 1]; ppl.next_id = bid; ppl.version += 1; self.pub.publish(ppl)
    def on_event(self, pe):
        if pe.priority == 0: return
        if pe.priority == 4 and self.beacons:
            bid = max(self.beacons); x, y, old = self.beacons[bid]
            pl = self.payload(bid, x, y, pe.event, 4, old.version + 1); pl.next_id = old.next_id
            self.beacons[bid] = (x, y, pl); self.pub.publish(pl); return
        self.pending.append((pe.event, pe.priority)); self.p_req.publish(UInt8(data=pe.priority))

def main():
    rclpy.init(); rclpy.spin(BeaconWriter())
