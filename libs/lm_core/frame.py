"""Frame translation. W = Writer frame, origin at B0 (entrance), x = entry heading.
Two paths: (a) chain beacons B0->Bi by (dir, dist) -> W coords; (b) W -> ENU -> WGS84."""
import math

def chain_positions(beacons: dict[int, tuple[int, float, float]]) -> dict[int, tuple[float, float]]:
    """beacons: id -> (prev_id, dir_deg_to_prev, dist_m). Returns id -> (x, y) in W, B0 at (0,0)."""
    pos = {0: (0.0, 0.0)}
    pending = dict(beacons); pending.pop(0, None)
    while pending:
        progressed = False
        for bid, (prev, d, r) in list(pending.items()):
            if prev in pos:
                px, py = pos[prev]; a = math.radians(d)     # dir points FROM bi TO prev
                pos[bid] = (px - r * math.cos(a), py - r * math.sin(a)); del pending[bid]; progressed = True
        if not progressed: break        # orphaned beacons (broken chain) -> failure case
    return pos

def w_to_wgs84(x, y, heading_deg, lat0, lon0):
    psi = math.radians(heading_deg)     # heading of W's x-axis, measured from East, CCW
    e = math.cos(psi) * x - math.sin(psi) * y; n = math.sin(psi) * x + math.cos(psi) * y
    return lat0 + n / 111_320.0, lon0 + e / (111_320.0 * math.cos(math.radians(lat0)))
