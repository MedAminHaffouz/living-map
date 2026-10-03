"""Confirms raw detections into Events: pose-tag + k-of-n confirm + spatial dedupe.

Zone: WRITER.
Inputs: writer/pose -> Pose2D, writer/det -> SensorDetection.
Outputs: writer/event -> Event.
Active in: EXPLORE.
Params read from config/wiring.yaml: k (confirmations required, default 3), n (sliding
window length, default 5), thresh (detection-probability threshold, default 0.5),
dedupe_r (spatial dedupe radius, m, default 2.0).
P1 status: per EventType, keeps a sliding window of the last n `p > thresh` hits; once
k of them are true and no existing emitted Event of the same type is within dedupe_r,
emits a new Event at the current pose. No co-occurrence severity boost yet.
P2 plan: same interface; real sensors replace SimScalarSensor upstream, no change here.
TODOs:
    - TODO: co-occurrence severity boost (e.g. FIRE+SMOKE at the same spot raises severity).
"""
import math
from core.module import Module
from core.zones import Zone
from contracts.messages import Event, Frame, MissionState as S

class Fusion(Module):
    """pose-tag + k-of-n confirm + spatial dedupe. Co-occurrence severity boost TODO."""
    ZONE = Zone.WRITER; INPUTS = ("writer/pose", "writer/det"); OUTPUTS = ("writer/event",); ACTIVE_IN = (S.EXPLORE,)
    def __init__(self, name, params=None):
        """Track latest pose, per-type hit-history windows, emitted Events, and the next event id."""
        super().__init__(name, params); self.pose = None; self.hits = {}; self.emitted = []; self.nid = 0
    def step(self, t, inbox):
        """Update the latest pose, then k-of-n confirm each detection into a deduped Event."""
        if inbox["writer/pose"]: self.pose = inbox["writer/pose"][-1]
        if self.pose is None: return
        out = []
        k, n, th, r = self.p.get("k", 3), self.p.get("n", 5), self.p.get("thresh", 0.5), self.p.get("dedupe_r", 2.0)
        for d in inbox["writer/det"]:
            h = self.hits.setdefault(d.type, []); h.append(d.p > th); del h[:-n]
            if sum(h) >= k and not any(e.type == d.type and math.hypot(e.x - self.pose.x, e.y - self.pose.y) < r for e in self.emitted):
                self.nid += 1
                e = Event(self.hdr(t, Frame.W), self.nid, d.type, severity=d.p, conf=d.conf * sum(h) / n, x=self.pose.x, y=self.pose.y)
                self.emitted.append(e); out.append(e)
        return {"writer/event": out}
