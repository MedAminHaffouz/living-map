"""lm_uros_conv.h: every wire struct survives wire -> micro-ROS -> wire, against rosidl-shaped structs built from the .msg files."""
import pathlib, shutil, subprocess, sys, pytest
R = pathlib.Path(__file__).resolve().parents[1]
GEN = R / "contracts/generated"
sys.path.insert(0, str(R / "tests"))
from rosidl_fake import headers, snake

def fake_rosidl():
    """What rosidl_generator_c emits for every lm_interfaces/msg/<Name>.h, concatenated."""
    h = headers(); return "\n".join(h.values()), list(h)

@pytest.mark.skipif(not shutil.which("gcc"), reason="gcc missing")
def test_roundtrip_all_messages(tmp_path):
    decls, names = fake_rosidl()
    body = []
    for n in names:
        s = snake(n)
        body += [f"    {{ lm_{s}_t a, b; lm_interfaces__msg__{n} r;",
                 "      for (size_t i = 0; i < sizeof a; i++) ((uint8_t *)&a)[i] = (uint8_t)(0x11 + i);",
                 "      memset(&b, 0, sizeof b); memset(&r, 0, sizeof r);",
                 f"      lm_ros_from_{s}(&r, &a); lm_{s}_from_ros(&b, &r);",
                 f'      if (memcmp(&a, &b, sizeof a)) {{ printf("FAIL {n}\\n"); fail = 1; }} else printf("ok {n}\\n"); }}']
    src = tmp_path / "t.c"
    src.write_text("#include <stdint.h>\n#include <stdio.h>\n#include <string.h>\n" + decls +
                   '#include "lm_uros_conv.h"\nint main(void) { int fail = 0;\n' + "\n".join(body) + "\n    return fail; }\n")
    exe = tmp_path / "t"
    subprocess.run(["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-I", GEN / "c", src, "-o", exe], check=True)
    out = subprocess.run([exe], capture_output=True, text=True, check=True).stdout
    assert out.count("ok ") == len(names) and len(names) > 0

@pytest.mark.skipif(not shutil.which("gcc"), reason="gcc missing")
def test_compiles_without_rosidl(tmp_path):
    """Firmware that includes no lm_interfaces header still compiles the converter header (all pairs guarded out)."""
    src = tmp_path / "t.c"; src.write_text('#include "lm_uros_conv.h"\nint main(void) { return 0; }\n')
    subprocess.run(["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-I", GEN / "c", src, "-o", tmp_path / "t"], check=True)
