"""Encodes DropCommands into BeaconPayloads, writes them, and uploads the mission log.

Zone: WRITER.
Inputs: writer/drop -> DropCommand, writer/event -> Event, writer/triage -> TriageDecision,
mission/state -> MissionStateMsg.
Outputs: beacon/write -> BeaconWrite (physical radio write), ona/upload -> MissionLog
(the only Writer egress to outside the ONA).
Active in: always (no ACTIVE_IN set; relies on mission/state to know when to upload).
Params read from config/wiring.yaml: none.
P1 status: assigns sequential beacon ids (B0 = entrance, id 0), computes bearing/distance
to the previous beacon from consecutive drop poses, tags the beacon with the oldest
pending event's type/priority when the drop reason is "event" (else JUNCTION/LOW), and
uploads the full mission log exactly once on the first tick mission/state == UPLOAD.
P2 plan: beacon/write becomes an actual RF write to the ESP32-C3 beacon firmware;
ona/upload becomes the real Writer->ONA handoff at the physical dock. Contract unchanged.
"""
import math
from core.module import Module
from core.zones import Zone
from contracts.messages import BeaconPayload, BeaconWrite, EventType, Priority, MissionLog, MissionState as S

class BeaconWriter(Module):
    """Encodes payload + keeps mission log; uploads log to ONA when state == UPLOAD."""
    ZONE = Zone.WRITER
    INPUTS = ("writer/drop", "writer/event", "writer/triage", "mission/state")
    OUTPUTS = ("beacon/write", "ona/upload")
    def __init__(self, name, params=None):
        """Track next beacon id, last drop pose, accumulated events/writes/priorities, and upload-once flag."""
        super().__init__(name, params); self.bid = -1; self.prev = None
        self.events, self.writes, self.prio, self.pending = [], [], {}, []; self.uploaded = False
    def step(self, t, inbox):
        """Encode each DropCommand into a BeaconWrite; upload the MissionLog once on entering UPLOAD."""
        self.events += inbox["writer/event"]; self.pending += inbox["writer/event"]
        for d in inbox["writer/triage"]: self.prio[d.event_id] = d.priority
        out = {"beacon/write": [], "ona/upload": []}
        for c in inbox["writer/drop"]:
            self.bid += 1
            ev = self.pending.pop(0) if (c.reason == "event" and self.pending) else None
            dx, dy = (c.x - self.prev[0], c.y - self.prev[1]) if self.prev else (0, 0)
            pl = BeaconPayload(self.bid, ev.type if ev else EventType.JUNCTION,
                               self.prio.get(ev.id, Priority.LOW) if ev else Priority.LOW,
                               int(math.degrees(math.atan2(-dy, -dx)) % 360), int(math.hypot(dx, dy) * 100),
                               int(t), 0, max(self.bid - 1, 0))
            w = BeaconWrite(self.hdr(t), pl, (c.x, c.y)); self.writes.append(w); out["beacon/write"].append(w)
            self.prev = (c.x, c.y)
        if any(s.state == S.UPLOAD for s in inbox["mission/state"]) and not self.uploaded:
            self.uploaded = True
            out["ona/upload"].append(MissionLog(self.hdr(t), tuple(self.events), tuple(self.writes)))
        return out
