"""ONA <-> CP link: failover cellular -> satellite -> back to cellular; SBD compact() situation within 300 bytes."""
import random, sys, pathlib, threading, yaml
from http.server import ThreadingHTTPServer
import pytest
R = pathlib.Path(__file__).resolve().parents[1]
sys.path[:0] = [str(R / "targets/ona_pc"), str(R / "targets/cp_pc"), str(R / "libs"), str(R / "contracts/generated/py")]
from ona.cp_link import Failover, HttpCellular, SatelliteSBD, compact, expand, SBD_MAX
import cp_server

class FakeLink:
    def __init__(self, name): self.name, self.up, self.calls, self.got = name, True, 0, []
    def send_situation(self, s):
        self.calls += 1
        if not self.up: raise OSError(f"{self.name} down")
        self.got.append(s)
    def fetch_mission(self):
        self.calls += 1
        if not self.up: raise OSError(f"{self.name} down")
        return {"mission_id": 1, "via": self.name}

def test_failover_cellular_satellite_and_back():
    t = [0.0]; cell, sat = FakeLink("cellular"), FakeLink("satellite")
    f = Failover([cell, sat], probe_s=30, clock=lambda: t[0])
    f.send_situation({"n": 1}); assert f.name == "cellular" and cell.got and not sat.got
    cell.up = False; t[0] = 1
    f.send_situation({"n": 2}); assert f.name == "satellite" and sat.got == [{"n": 2}]
    assert f.fetch_mission()["via"] == "satellite"
    cell.up = True; t[0] = 10; before = cell.calls                     # cellular back, but no probe yet
    f.send_situation({"n": 3}); assert f.name == "satellite" and cell.calls == before
    t[0] = 31.5                                                         # probe_s elapsed since the failed try
    f.send_situation({"n": 4}); assert f.name == "cellular" and cell.got[-1] == {"n": 4}
    cell.up = sat.up = False; t[0] = 40
    with pytest.raises(OSError, match="all CP links down"): f.send_situation({"n": 5})

def test_failed_probe_stays_on_backup_and_waits_again():
    t = [0.0]; cell, sat = FakeLink("cellular"), FakeLink("satellite")
    f = Failover([cell, sat], probe_s=30, clock=lambda: t[0])
    cell.up = False; f.send_situation({}); assert f.name == "satellite"
    t[0] = 30; f.send_situation({}); assert f.name == "satellite"       # probed, still down
    n = cell.calls; t[0] = 45; f.send_situation({}); assert cell.calls == n   # next probe only at 60

ENTRANCE = {"lat": 36.843, "lon": 10.197}

def beacon(i, what, prio):
    return {"id": i, "what": what, "prio": prio, "age_s": 10 * i, "state": "FRESH", "version": 0,
            "lat": ENTRANCE["lat"] + 1e-5 * i, "lon": ENTRANCE["lon"] - 2e-5 * i, "x": i, "y": 0}

def test_compact_61_events_immediate_first():
    rnd = random.Random(7)
    bs = [beacon(i, rnd.choice(["FIRE", "GAS", "PERSON", "SMOKE"]), rnd.choice(["IGNORE", "LOW", "NORMAL", "HIGH"])) for i in range(61)]
    bs[42] = beacon(42, "PERSON", "IMMEDIATE")
    bs += [beacon(100 + i, "JUNCTION", "IMMEDIATE") for i in range(5)] + [{"id": 200, "orphan": True}]
    blob = compact({"beacons": bs}, ENTRANCE)
    assert len(blob) <= SBD_MAX
    got = expand(blob)["beacons"]
    assert got[0]["id"] == 42 and got[0]["prio"] == "IMMEDIATE"
    assert all(b["what"] != "JUNCTION" for b in got)
    rank = ["IMMEDIATE", "HIGH", "NORMAL", "LOW", "IGNORE"]
    ranks = [rank.index(b["prio"]) for b in got]
    assert ranks == sorted(ranks)                                        # most urgent first
    events = [b for b in bs if not b.get("orphan") and b["what"] != "JUNCTION"]
    cut = rank.index(got[-1]["prio"])
    assert all(any(g["id"] == b["id"] for g in got) for b in events if rank.index(b["prio"]) < cut)   # cut only from the least urgent
    b = next(x for x in bs if x["id"] == got[1]["id"])
    assert abs(got[1]["lat"] - b["lat"]) < 1e-6 and abs(got[1]["lon"] - b["lon"]) < 1e-6 and got[1]["age_s"] == b["age_s"]

def test_satellite_stub_builds_payload_then_raises():
    s = SatelliteSBD("/dev/null", ENTRANCE)
    with pytest.raises(OSError): s.send_situation({"beacons": [beacon(1, "FIRE", "HIGH")]})
    assert 0 < len(s.last_payload) <= SBD_MAX and expand(s.last_payload)["beacons"][0]["what"] == "FIRE"
    with pytest.raises(OSError): s.fetch_mission()

def test_http_cellular_against_cp_server():
    srv = ThreadingHTTPServer(("127.0.0.1", 0), cp_server.H); threading.Thread(target=srv.serve_forever, daemon=True).start()
    try:
        cp_server.STATE.update(situation=None, mission={"mission_id": 7, "steps": []})
        link = HttpCellular(f"http://127.0.0.1:{srv.server_port}")
        link.send_situation({"beacons": []})
        assert cp_server.STATE["situation"] == {"beacons": []} and link.fetch_mission()["mission_id"] == 7
    finally: srv.shutdown()
    with pytest.raises(OSError): HttpCellular("http://127.0.0.1:9", timeout_s=0.5).fetch_mission()

def test_main_wires_cellular_then_satellite():
    pytest.importorskip("serial")
    from ona.main import cp_links
    area = yaml.safe_load((R / "targets/ona_pc/config/area.yaml").read_text())
    assert {"gateway_port", "cp_url", "sat_port"} <= set(area)
    f = cp_links(area)
    assert [l.name for l in f.links] == ["cellular", "satellite"] and f.probe_s == 30
