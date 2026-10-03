"""Pi <-> radio ESP32 (USB-serial). The only node that touches the beacon radio.
Down: /beacon/write (BeaconPayload) -> ESP-NOW
Up:   BeaconObs -> /beacon/obs (incl. RSSI, consumed by beacon_dropper) ; BeaconAck -> /beacon/ack"""
from . import _paths  # noqa
import serial, rclpy
from rclpy.node import Node
import lm_interfaces.msg as R
import lm_msgs as M
from lm_core.link import Decoder, encode
from ._conv import to_ros, from_ros

class RadioBridge(Node):
    def __init__(self):
        super().__init__("radio_bridge")
        self.ser = serial.Serial(self.declare_parameter("port", "/dev/ttyUSB0").value, 921600, timeout=0)
        self.dec = Decoder()
        self.p_obs = self.create_publisher(R.BeaconObs, "/beacon/obs", 20)
        self.p_ack = self.create_publisher(R.BeaconAck, "/beacon/ack", 10)
        self.create_subscription(R.BeaconPayload, "/beacon/write",
                                 lambda m: self.ser.write(encode(M.BeaconPayload.ID, from_ros(m, M.BeaconPayload).pack())), 10)
        self.create_timer(0.005, self.poll)
    def poll(self):
        for mid, p in self.dec.feed(self.ser.read(512)):
            if mid == M.BeaconObs.ID and len(p) == M.BeaconObs.SIZE: self.p_obs.publish(to_ros(M.BeaconObs.unpack(p), R.BeaconObs))
            elif mid == M.BeaconAck.ID and len(p) == M.BeaconAck.SIZE: self.p_ack.publish(to_ros(M.BeaconAck.unpack(p), R.BeaconAck))

def main():
    rclpy.init(); rclpy.spin(RadioBridge())
