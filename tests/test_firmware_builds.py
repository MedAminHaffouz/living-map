"""Firmware app code compiles + runs 10k ticks on the host (catches contract drift in C)."""
import pathlib, shutil, subprocess, pytest
R = pathlib.Path(__file__).resolve().parents[1]
INC = ["-I", R / "libs/lm_embedded", "-I", R / "contracts/generated/c"]
LIB = [R / "libs/lm_embedded/lm_link.c", R / "libs/lm_embedded/lm_aging.c", R / "tests/c/board_stub.c", R / "tests/c/main_stub.c"]

@pytest.mark.skipif(not shutil.which("gcc"), reason="gcc missing")
@pytest.mark.parametrize("target", ["writer_stm", "executor_stm"])
def test_builds_and_runs(tmp_path, target):
    app = R / "targets" / target / "app"
    exe = tmp_path / target
    subprocess.run(["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter", *INC, "-I", app,
                    *sorted(app.glob("*.c")), *LIB, "-lm", "-o", exe], check=True)
    subprocess.run([exe], check=True, timeout=10)
