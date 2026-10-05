"""LoRa firmware on the host: lora_gateway logic against fake links, and both main.cpp glue files compile
against the Arduino API stubs (no PlatformIO needed; catches drift between glue and libs/lm_embedded)."""
import pathlib, shutil, subprocess, pytest
R = pathlib.Path(__file__).resolve().parents[1]
E, GEN, GW = R / "libs/lm_embedded", R / "contracts/generated/c", R / "targets/lora_gateway/src"
INC = ["-I", E, "-I", GEN]
WARN = ["-Wall", "-Wextra", "-Werror"]

@pytest.mark.skipif(not shutil.which("gcc"), reason="gcc missing")
def test_gateway_logic(tmp_path):
    exe = tmp_path / "gw"
    subprocess.run(["gcc", "-std=c11", *WARN, *INC, "-I", GW, R / "tests/c/test_gateway.c", GW / "gateway.c",
                    E / "lm_link.c", E / "lm_lora_sx127x.c", E / "lm_slots.c", E / "lm_txq.c", "-o", exe], check=True)
    r = subprocess.run([exe], capture_output=True, text=True, timeout=10)
    assert r.returncode == 0 and "ALL OK" in r.stdout, r.stdout

@pytest.mark.skipif(not shutil.which("g++"), reason="g++ missing")
@pytest.mark.parametrize("fw,defs", [("beacon_fw", ["-DBEACON_ID=3"]), ("lora_gateway", [])])
def test_glue_compiles(tmp_path, fw, defs):
    src = R / "targets" / fw / "src"
    subprocess.run(["g++", "-std=gnu++17", *WARN, *defs, "-I", R / "tests/arduino_stub", *INC, "-I", src,
                    "-c", src / "main.cpp", "-o", tmp_path / "main.o"], check=True)

def test_build_src_filter_points_at_real_files():
    seen = 0
    for ini in (R / "targets").glob("*/platformio.ini"):
        for line in ini.read_text().splitlines():
            line = line.strip()
            if line.startswith("+<../"):
                assert (ini.parent / "src" / line[2:-1]).resolve().is_file(), (ini, line); seen += 1
    assert seen >= 9   # beacon_fw 4 + lora_gateway 5
