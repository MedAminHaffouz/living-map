"""Beacons reader: radio ESP (usb_ona) -> latest BeaconObs per id (highest version wins) + ActionReports."""
from . import _paths  # noqa
import lm_msgs as M
from lm_core.link import Decoder

class BeaconsReader:
    def __init__(self, stream):
        self.stream, self.dec = stream, Decoder()
        self.beacons: dict[int, M.BeaconObs] = {}; self.reports: list[M.ActionReport] = []
    def poll(self):
        for mid, p in self.dec.feed(self.stream.read(512) or b""):
            if mid == M.BeaconObs.ID and len(p) == M.BeaconObs.SIZE:
                o = M.BeaconObs.unpack(p); cur = self.beacons.get(o.id)
                if cur is None or o.version >= cur.version: self.beacons[o.id] = o
            elif mid == M.ActionReport.ID and len(p) == M.ActionReport.SIZE:
                self.reports.append(M.ActionReport.unpack(p))
