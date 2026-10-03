"""ONA end-to-end without hardware: fake radio stream -> situation -> CP -> mission -> brief frames."""
import io, sys, pathlib, json, threading, urllib.request
from http.server import ThreadingHTTPServer
R = pathlib.Path(__file__).resolve().parents[1]
sys.path[:0] = [str(R / "targets/ona_pc"), str(R / "targets/cp_pc"), str(R / "libs"), str(R / "contracts/generated/py")]
import lm_msgs as M
from lm_core.link import encode, Decoder
from ona.beacons_reader import BeaconsReader
from ona.situation_debrief import build
from ona.mission_debrief import frames
import cp_server

AREA = {"entrance": {"lat": 36.843, "lon": 10.197}, "heading_deg": 0.0}

def obs(i, prev, d, dist, what=4, ver=0, age=5):
    return encode(M.BeaconObs.ID, M.BeaconObs(i, what, 2, 230, d, dist, prev, 255, age, ver, 0, -60).pack())

def test_situation_from_beacon_chain():
    stream = io.BytesIO(obs(0, 0, 0, 0) + obs(1, 0, 180, 400) + obs(2, 1, 180, 300, what=1) + obs(2, 1, 180, 300, what=1, ver=1, age=500))
    rd = BeaconsReader(stream); rd.poll()
    s = build(rd.beacons, AREA)["beacons"]
    b2 = next(b for b in s if b["id"] == 2)
    assert b2["what"] == "FIRE" and b2["version"] == 1 and abs(b2["x"] - 7.0) < 1e-6 and b2["state"] == "STALE"
    assert b2["lon"] > AREA["entrance"]["lon"]          # 7 m east of the entrance

def test_mission_to_brief_frames():
    m = {"mission_id": 9, "steps": [{"beacon_id": 2, "action": "EXTINGUISH", "event": "FIRE"}]}
    got = list(Decoder().feed(b"".join(frames(m))))
    assert [g[0] for g in got] == [M.BriefHeader.ID, M.BriefStep.ID]
    assert M.BriefStep.unpack(got[1][1]).action == M.Action.EXTINGUISH

def test_cp_roundtrip():
    srv = ThreadingHTTPServer(("127.0.0.1", 0), cp_server.H); threading.Thread(target=srv.serve_forever, daemon=True).start()
    url = f"http://127.0.0.1:{srv.server_port}"
    req = urllib.request.Request(url + "/mission", data=json.dumps({"mission_id": 1, "steps": []}).encode(), method="POST",
                                 headers={"Content-Type": "application/json"})
    urllib.request.urlopen(req).read()
    assert json.loads(urllib.request.urlopen(url + "/mission").read())["mission_id"] == 1
    srv.shutdown()
