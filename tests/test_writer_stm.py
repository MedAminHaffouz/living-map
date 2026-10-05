"""Writer STM: app behaviour on the host (uplink stub + stub Ra-02), and uros_app.c against the micro-ROS API stubs."""
import pathlib, re, shutil, subprocess, sys, pytest
R = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(R / "tests"))
from rosidl_fake import write_include_tree
E, APP = R / "libs/lm_embedded", R / "targets/writer_stm/app"
INC = ["-I", E, "-I", R / "contracts/generated/c", "-I", APP]
CC = ["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror"]
LIBS = [E / f for f in ("lm_link.c", "lm_lora_sx127x.c", "lm_slots.c")]

def app_sources(): return [f for f in sorted(APP.glob("*.c")) if f.name != "uros_app.c"]

@pytest.mark.skipif(not shutil.which("gcc"), reason="gcc missing")
def test_writer_app_behaviour(tmp_path):
    exe = tmp_path / "w"
    subprocess.run([*CC, *INC, R / "tests/c/test_writer.c", *app_sources(), R / "tests/c/uplink_stub.c",
                    R / "tests/c/board_stub.c", *LIBS, "-lm", "-o", exe], check=True)
    r = subprocess.run([exe], capture_output=True, text=True, timeout=20)
    assert r.returncode == 0 and "ALL OK" in r.stdout, r.stdout

@pytest.mark.skipif(not shutil.which("gcc"), reason="gcc missing")
def test_uros_app_against_microros_api(tmp_path):
    inc = write_include_tree(tmp_path / "inc")
    subprocess.run([*CC, "-fsyntax-only", "-I", R / "tests/uros_stub", "-I", inc, *INC, APP / "uros_app.c"], check=True)

def test_only_uros_app_includes_ros():
    ros = re.compile(r'#include\s*[<"](rcl|rclc|rmw|rmw_microros|rosidl\w*|geometry_msgs|lm_interfaces)/')
    offenders = [f.name for f in APP.glob("*.[ch]") if ros.search(f.read_text()) and f.name != "uros_app.c"]
    assert offenders == []

def test_calibrate_srv_matches_stub():
    srv = (R / "targets/writer_pi/ros2_ws/src/lm_interfaces/srv/Calibrate.srv").read_text()
    req, res = (part.split("\n") for part in srv.split("---"))
    fields = lambda lines: [tuple(l.split("#")[0].split()) for l in lines if l.split("#")[0].strip()]
    assert fields(req) == [("uint8", "target"), ("uint8", "op")] and fields(res) == [("bool", "ok")]
