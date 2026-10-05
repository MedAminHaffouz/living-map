"""ONA <-> CP link. One protocol, swappable transports:
  HttpCellular  primary: JSON over HTTP (POST /situation, GET /mission)
  SatelliteSBD  backup: Iridium SBD, 300-byte budget per message -> compact() binary situation (driver TODO)
  Failover      tries links in order, sticks to the one that works, re-probes the primary every probe_s.
Every link raises OSError when it can't deliver; Failover only moves on OSError."""
from . import _paths  # noqa
import json, struct, time, urllib.request
from typing import Protocol
import lm_msgs as M

class CpLink(Protocol):
    name: str
    def send_situation(self, situation: dict) -> None: ...
    def fetch_mission(self) -> dict | None: ...

class HttpCellular:
    name = "cellular"
    def __init__(self, url: str, timeout_s: float = 3.0): self.url, self.timeout = url.rstrip("/"), timeout_s
    def _http(self, path, data=None):
        req = urllib.request.Request(self.url + path, data=json.dumps(data).encode() if data is not None else None,
                                     headers={"Content-Type": "application/json"}, method="POST" if data is not None else "GET")
        with urllib.request.urlopen(req, timeout=self.timeout) as r: return json.loads(r.read() or b"null")
    def send_situation(self, situation): self._http("/situation", situation)
    def fetch_mission(self): return self._http("/mission")

# ---- satellite: compact binary situation (Iridium SBD MO max is 340 B; we budget 300) ----
SBD_MAX = 300
HDR = struct.Struct("<BBii")     # version, count, entrance lat/lon in 1e-6 deg
EVT = struct.Struct("<BBBHhh")   # id, what<<4|prio, state, age_s (capped), dlat/dlon from entrance in 1e-6 deg
SBD_VERSION = 1

def _events(situation):
    ev = [b for b in situation["beacons"] if not b.get("orphan") and b["what"] not in ("JUNCTION", "NONE")]
    return sorted(ev, key=lambda b: (-M.Priority[b["prio"]], b["id"]))   # IMMEDIATE > ... > IGNORE

def compact(situation: dict, entrance: dict, limit: int = SBD_MAX) -> bytes:
    """Events only (no JUNCTION), most urgent first; whatever doesn't fit is cut from the least urgent end."""
    lat0, lon0 = round(entrance["lat"] * 1e6), round(entrance["lon"] * 1e6)
    body = []
    for b in _events(situation)[: (limit - HDR.size) // EVT.size]:
        d = lambda v, v0: max(-32768, min(32767, round(v * 1e6) - v0))
        body.append(EVT.pack(b["id"], M.EventType[b["what"]] << 4 | M.Priority[b["prio"]], M.AgeState[b["state"]], min(b["age_s"], 0xFFFF),
                             d(b["lat"], lat0), d(b["lon"], lon0)))
    return HDR.pack(SBD_VERSION, len(body), lat0, lon0) + b"".join(body)

def expand(blob: bytes) -> dict:
    """CP side of compact(): back to the situation JSON shape (positions to ~0.1 m)."""
    ver, n, lat0, lon0 = HDR.unpack_from(blob)
    if ver != SBD_VERSION: raise ValueError(f"SBD version {ver}")
    out = []
    for i in range(n):
        bid, wp, st, age, dlat, dlon = EVT.unpack_from(blob, HDR.size + i * EVT.size)
        out.append({"id": bid, "what": M.EventType(wp >> 4).name, "prio": M.Priority(wp & 0x0F).name, "state": M.AgeState(st).name,
                    "age_s": age, "lat": (lat0 + dlat) / 1e6, "lon": (lon0 + dlon) / 1e6})
    return {"beacons": out}

class SatelliteSBD:
    name = "satellite"
    def __init__(self, port: str, entrance: dict | None = None):
        self.port, self.entrance = port, entrance or {"lat": 0.0, "lon": 0.0}
        self.last_payload = b""
    def send_situation(self, situation):
        self.last_payload = compact(situation, self.entrance)
        raise OSError(f"SBD modem on {self.port}: driver not implemented")   # TODO: AT+SBDWB / AT+SBDIX (RockBLOCK, 9603)
    def fetch_mission(self):
        raise OSError(f"SBD modem on {self.port}: driver not implemented")   # TODO: MT message via AT+SBDRB

class Failover:
    """links[0] is the primary. While on a backup, the primary is tried again once every probe_s."""
    def __init__(self, links: list, probe_s: float = 30.0, clock=time.monotonic):
        self.links, self.probe_s, self.clock = list(links), probe_s, clock
        self.active, self.t_primary = 0, None
    @property
    def name(self): return self.links[self.active].name

    def _order(self):
        n, now = len(self.links), self.clock()
        if self.active == 0 or self.t_primary is None or now - self.t_primary >= self.probe_s:
            self.t_primary = now
            return [0] + [i for i in range(1, n)]
        return [i for i in range(self.active, n)] + [i for i in range(1, self.active)]   # primary skipped until probe

    def _call(self, op, *args):
        errors = []
        for i in self._order():
            try: r = getattr(self.links[i], op)(*args)
            except OSError as e: errors.append(f"{self.links[i].name}: {e}"); continue
            self.active = i
            return r
        raise OSError("all CP links down: " + "; ".join(errors))

    def send_situation(self, situation): return self._call("send_situation", situation)
    def fetch_mission(self): return self._call("fetch_mission")
