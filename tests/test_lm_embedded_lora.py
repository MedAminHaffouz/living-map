"""libs/lm_embedded LoRa stack on the host: fake SX127x, TDMA slots, beacon core, UART link, airtime vs Semtech formula."""
import math, pathlib, shutil, subprocess, pytest
R = pathlib.Path(__file__).resolve().parents[1]
E = R / "libs/lm_embedded"
SRC = [E / f for f in ("lm_link.c", "lm_lora_sx127x.c", "lm_link_uart.c", "lm_slots.c", "lm_beacon_core.c")]

@pytest.fixture(scope="module")
def exe(tmp_path_factory):
    if not shutil.which("gcc"): pytest.skip("gcc missing")
    out = tmp_path_factory.mktemp("lora") / "test_lora"
    subprocess.run(["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-I", E, "-I", R / "contracts/generated/c",
                    R / "tests/c/test_lora.c", *SRC, "-o", out], check=True)
    return out

def test_fake_radio_beacon_slots_uart(exe):
    r = subprocess.run([exe], capture_output=True, text=True, timeout=20)
    assert r.returncode == 0 and "ALL OK" in r.stdout, r.stdout

def semtech_ms(sf, bw, cr, n, preamble=8):
    """AN1200.13, explicit header, CRC on, LDRO when SF >= 11 and BW <= 125 kHz."""
    de = 1 if sf >= 11 and bw <= 125000 else 0
    t_sym = (2 ** sf) / bw
    payload = 8 + max(math.ceil((8 * n - 4 * sf + 28 + 16) / (4 * (sf - 2 * de))) * cr, 0)
    return (preamble + 4.25 + payload) * t_sym * 1000

def test_airtime_matches_semtech(exe):
    rows = [list(map(int, l.split())) for l in subprocess.run([exe, "airtime"], capture_output=True, text=True, check=True).stdout.split("\n") if l]
    assert len(rows) > 100
    for sf, bw, cr, n, ms in rows:
        assert ms == math.ceil(semtech_ms(sf, bw, cr, n) - 1e-9), (sf, bw, cr, n, ms)
