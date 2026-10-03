"""Situation debrief: beacons -> W coords (chain B0->Bi by dir/dist) -> WGS84 -> aging state -> JSON for CP."""
from . import _paths  # noqa
from lm_core.frame import chain_positions, w_to_wgs84
from lm_core import aging
import lm_msgs as M

def build(beacons: dict, area: dict) -> dict:
    chain = {b.id: (b.prev_id, b.dir_deg, b.dist_cm / 100) for b in beacons.values()}
    pos = chain_positions(chain); out = []
    for b in beacons.values():
        if b.id not in pos: out.append({"id": b.id, "orphan": True}); continue   # broken chain: failure case
        lat, lon = w_to_wgs84(*pos[b.id], area["heading_deg"], area["entrance"]["lat"], area["entrance"]["lon"])
        out.append({"id": b.id, "what": M.EventType(b.what).name, "prio": M.Priority(b.prio).name,
                    "age_s": b.age_s, "state": M.AgeState(aging.state(b.conf / 255, b.age_s, b.what, b.flags)).name,
                    "version": b.version, "lat": lat, "lon": lon, "x": pos[b.id][0], "y": pos[b.id][1]})
    return {"beacons": sorted(out, key=lambda d: d["id"])}
