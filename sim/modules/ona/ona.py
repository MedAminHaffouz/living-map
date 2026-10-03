"""ONA (Outside Network Area): Receive+Translate+Carry (Translate) and Brief (BriefOut).

Zone: ONA.
Inputs: Translate: ona/upload -> MissionLog (the only inbound crossing from WRITER).
    BriefOut: cp/plan -> Brief (from CP).
Outputs: Translate: cp/situation -> GeoEvent (to CP). BriefOut: executor/brief -> Brief
    (the only outbound crossing to EXECUTOR).
Active in: always (no ACTIVE_IN on either class).
Params read from config/wiring.yaml: Translate: heading_deg (entrance heading, deg,
required), entrance_lat (deg, required), entrance_lon (deg, required). BriefOut: none.
P1 status: Translate converts each MissionLog's Events (frame W, asserted) to WGS84 via
a fixed-heading rotation into ENU then a flat-earth equirectangular projection (accurate
under ~1 km); every translated event starts at Age.FRESH. "Carry" (to a far Command
Post) is modeled as the cp/situation topic hop, not a real transport. BriefOut is a pure
pass-through of cp/plan to executor/brief.
P2 plan: same math, but Translate reads a real exit pose/heading instead of a static
heading_deg param; "Carry" becomes a real long-haul link (LoRa/satcom/etc.) to the
physical Command Post. Topics/types unchanged.
TODOs:
    - TODO: heading_deg is a fixed param; P2 should derive heading from the Writer's own exit pose.
"""
import math
from core.module import Module
from core.zones import Zone
from contracts.messages import GeoEvent, Frame, Age

class Translate(Module):
    """Receive + Translate + Carry. W -> ENU (heading psi, origin = entrance) -> WGS84 (flat-earth, fine < 1 km)."""
    ZONE = Zone.ONA; INPUTS = ("ona/upload",); OUTPUTS = ("cp/situation",)
    def to_wgs84(self, x, y):
        """Rotate a frame-W point (x, y) (m) by heading_deg into ENU, then flat-earth project to (lat, lon) (deg)."""
        psi = math.radians(self.p["heading_deg"])
        e = math.cos(psi) * x - math.sin(psi) * y; n = math.sin(psi) * x + math.cos(psi) * y
        lat0, lon0 = self.p["entrance_lat"], self.p["entrance_lon"]
        return lat0 + n / 111_320, lon0 + e / (111_320 * math.cos(math.radians(lat0)))
    def step(self, t, inbox):
        """Translate every Event in each uploaded MissionLog to a fresh GeoEvent."""
        out = []
        for log in inbox["ona/upload"]:
            for ev in log.events:
                assert ev.header.frame == Frame.W, "translate expects frame W"
                lat, lon = self.to_wgs84(ev.x, ev.y)
                out.append(GeoEvent(self.hdr(t, Frame.WGS84), ev, lat, lon, Age.FRESH))
        return {"cp/situation": out}

class BriefOut(Module):
    """Relays the Command Post's approved plan to the Executor at the dock."""
    ZONE = Zone.ONA; INPUTS = ("cp/plan",); OUTPUTS = ("executor/brief",)
    def step(self, t, inbox):
        """Pass every cp/plan Brief straight through to executor/brief."""
        return {"executor/brief": list(inbox["cp/plan"])}
