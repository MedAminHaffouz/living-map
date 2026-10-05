"""lm_uros_conv.h: every wire struct survives wire -> micro-ROS -> wire, against rosidl-shaped structs built from the .msg files."""
import pathlib, re, shutil, subprocess, pytest
R = pathlib.Path(__file__).resolve().parents[1]
GEN = R / "contracts/generated"
CTYPE = {"uint8": "uint8_t", "int8": "int8_t", "uint16": "uint16_t", "int16": "int16_t",
         "uint32": "uint32_t", "int32": "int32_t", "float32": "float"}

def snake(n): return re.sub(r"(?<!^)([A-Z])", r"_\1", n).lower()

def fake_rosidl():
    """What rosidl_generator_c emits for lm_interfaces/msg/<Name>.h (plain, NOT packed), plus its include guard."""
    L, names = [], []
    for f in sorted((GEN / "ros/msg").glob("*.msg")):
        n = f.stem; names.append(n)
        flds = [l.split("#")[0].split() for l in f.read_text().splitlines() if l.strip() and not l.startswith("#")]
        L += [f"#define LM_INTERFACES__MSG__{snake(n).upper()}_H_", "typedef struct {"]
        L += [f"    {CTYPE[t]} {name};" for t, name in flds] + [f"}} lm_interfaces__msg__{n};", ""]
    return "\n".join(L), names

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
