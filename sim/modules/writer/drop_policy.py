"""Decides where to drop a beacon.

Zone: WRITER.
Inputs: writer/pose -> Pose2D, writer/triage -> TriageDecision.
Outputs: writer/drop -> DropCommand.
Active in: EXPLORE.
Params read from config/wiring.yaml: d_max (max spacing between beacons, m, default 4.0).
P1 status: location-based only — drops at entry (B0), then every d_max metres of travel
("spacing"), and once more at the current pose for any non-ignored triage decision when
not already dropping for spacing ("event"). "rssi"-reason drops (radio-link-based
spacing) are not implemented.
P2 plan: same DropCommand contract; d_max-based spacing may be augmented with live RSSI
from the beacon radio link once that exists in hardware.
TODOs:
    - TODO: "rssi" drop reason mentioned in contracts/messages.py DropCommand.reason is never emitted.
"""
import math
from core.module import Module
from core.zones import Zone
from contracts.messages import DropCommand, Frame, MissionState as S

class DropPolicy(Module):
    """Location-based: B0 at entry, then every d_max metres, and at every non-ignored event."""
    ZONE = Zone.WRITER; INPUTS = ("writer/pose", "writer/triage"); OUTPUTS = ("writer/drop",)
    ACTIVE_IN = (S.EXPLORE,)
    def __init__(self, name, params=None):
        """Track the pose of the last drop and the latest known pose."""
        super().__init__(name, params); self.last = None; self.pose = None
    def step(self, t, inbox):
        """Emit a DropCommand for entry/spacing and/or a non-ignored triage event at the current pose."""
        if inbox["writer/pose"]: self.pose = inbox["writer/pose"][-1]
        if self.pose is None: return
        out = []
        far = self.last is None or math.hypot(self.pose.x - self.last[0], self.pose.y - self.last[1]) > self.p.get("d_max", 4.0)
        if far: out.append(DropCommand(self.hdr(t, Frame.W), self.pose.x, self.pose.y, "entry" if self.last is None else "spacing"))
        if any(d.priority > 0 for d in inbox["writer/triage"]) and not far:
            out.append(DropCommand(self.hdr(t, Frame.W), self.pose.x, self.pose.y, "event"))
        if out: self.last = (self.pose.x, self.pose.y)
        return {"writer/drop": out}
