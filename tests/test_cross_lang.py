"""Python encoder <-> C decoder agree on framing, struct layout and aging."""
import json, pathlib, shutil, subprocess, sys
import pytest
R = pathlib.Path(__file__).resolve().parents[1]
sys.path[:0] = [str(R / "libs"), str(R / "contracts/generated/py")]
from lm_core.link import encode, Decoder
from lm_core import aging
import lm_msgs as M

def test_python_roundtrip():
    m = M.BeaconPayload(id=7, what=1, prio=3, conf=200, dir_deg=90, dist_cm=350, prev_id=6, next_id=255, age_s=40, version=2, flags=0)
    d = Decoder(); out = list(d.feed(b"\x00junk" + encode(m.ID, m.pack()) + b"\xA5"))
    assert out == [(m.ID, m.pack())] and M.BeaconPayload.unpack(out[0][1]) == m

def test_corrupted_frame_dropped():
    f = bytearray(encode(0x10, M.SensorDet(type=1).pack())); f[6] ^= 0xFF
    assert list(Decoder().feed(bytes(f))) == []

@pytest.mark.skipif(not shutil.which("gcc"), reason="gcc missing")
def test_c_decodes_python_frames(tmp_path):
    exe = tmp_path / "x"
    subprocess.run(["gcc", "-std=c11", "-Wall", "-Werror", "-I", R / "libs/lm_embedded", "-I", R / "contracts/generated/c",
                    R / "tests/c/test_cross.c", R / "libs/lm_embedded/lm_link.c", R / "libs/lm_embedded/lm_aging.c", "-lm", "-o", exe], check=True)
    v = json.loads((R / "contracts/generated/test_vectors.json").read_text())
    out = subprocess.run([exe, v["SensorDet"], v["BeaconPayload"]], capture_output=True, text=True, check=True).stdout
    assert "SensorDet type=1 detected=1 value=61.5 conf=200 t_ms=123456" in out
    py_state = aging.state(180 / 255, 17, 3, 2)
    assert f"BeaconPayload id=3 what=3 prio=4 dir=270 dist=412 age=17 ver=1 state={py_state}" in out
