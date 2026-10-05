"""Firmware app code compiles + runs 10k ticks on the host (catches contract drift in C)."""
import pathlib, shutil, subprocess, pytest
R = pathlib.Path(__file__).resolve().parents[1]
E = R / "libs/lm_embedded"
INC = ["-I", E, "-I", R / "contracts/generated/c"]
LIB = [E / "lm_link.c", E / "lm_aging.c", R / "tests/c/board_stub.c", R / "tests/c/main_stub.c"]
LORA = [E / "lm_lora_sx127x.c", E / "lm_slots.c", E / "lm_txq.c"]
CC = ["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter"]
BUILDS = {"writer_stm": ([], LORA + [R / "tests/c/uplink_stub.c"]),   # uros_app.c (micro-ROS) replaced by the uplink stub
          "executor_stm[EX_FIRE]": (["-DEXECUTOR_TYPE=EX_FIRE"], LORA),
          "executor_stm[EX_MED]": (["-DEXECUTOR_TYPE=EX_MED"], LORA)}

@pytest.mark.skipif(not shutil.which("gcc"), reason="gcc missing")
@pytest.mark.parametrize("build", BUILDS)
def test_builds_and_runs(tmp_path, build):
    defs, libs = BUILDS[build]
    app = R / "targets" / build.split("[")[0] / "app"
    exe = tmp_path / "fw"
    subprocess.run([*CC, *defs, *INC, "-I", app, *sorted(f for f in app.glob("*.c") if f.name != "uros_app.c"), *LIB, *libs, "-lm", "-o", exe], check=True)
    subprocess.run([exe], check=True, timeout=10)

@pytest.mark.skipif(not shutil.which("gcc"), reason="gcc missing")
@pytest.mark.parametrize("defs", [[], ["-DEXECUTOR_TYPE=3"]])
def test_executor_type_required(defs):
    app = R / "targets/executor_stm/app"
    r = subprocess.run([*CC, *defs, *INC, "-I", app, "-fsyntax-only", app / "app.c"], capture_output=True, text=True)
    assert r.returncode != 0 and "EXECUTOR_TYPE=EX_FIRE or -DEXECUTOR_TYPE=EX_MED" in r.stderr

@pytest.mark.skipif(not shutil.which("gcc"), reason="gcc missing")
@pytest.mark.parametrize("ex_type", ["EX_FIRE", "EX_MED"])
def test_executor_acts_only_on_its_capability(tmp_path, ex_type):
    app = R / "targets/executor_stm/app"
    srcs = [f for f in sorted(app.glob("*.c")) if f.name != "app.c"]       # app.c = radio glue, replaced by the test's ex_tx
    exe = tmp_path / "ex"
    subprocess.run([*CC, f"-DEXECUTOR_TYPE={ex_type}", *INC, "-I", app, R / "tests/c/test_executor.c", *srcs,
                    E / "lm_aging.c", "-lm", "-o", exe], check=True)
    r = subprocess.run([exe], capture_output=True, text=True, timeout=10)
    assert r.returncode == 0 and "ALL OK" in r.stdout, r.stdout
