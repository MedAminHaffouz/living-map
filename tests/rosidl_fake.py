"""rosidl_generator_c look-alikes built from the generated .msg files (plain structs, NOT packed, real include guards).
Used to compile lm_uros_conv.h and uros_app.c on a host without ROS / micro-ROS."""
import pathlib, re
R = pathlib.Path(__file__).resolve().parents[1]
MSG_DIR = R / "contracts/generated/ros/msg"
CTYPE = {"uint8": "uint8_t", "int8": "int8_t", "uint16": "uint16_t", "int16": "int16_t",
         "uint32": "uint32_t", "int32": "int32_t", "float32": "float"}

def snake(n): return re.sub(r"(?<!^)([A-Z])", r"_\1", n).lower()

def headers():
    """{Name: header text} for lm_interfaces/msg/<snake>.h"""
    out = {}
    for f in sorted(MSG_DIR.glob("*.msg")):
        n = f.stem
        flds = [l.split("#")[0].split() for l in f.read_text().splitlines() if l.strip() and not l.startswith("#")]
        out[n] = "\n".join([f"#ifndef LM_INTERFACES__MSG__{snake(n).upper()}_H_", f"#define LM_INTERFACES__MSG__{snake(n).upper()}_H_",
                            "#include <stdint.h>", "typedef struct {", *[f"    {CTYPE[t]} {name};" for t, name in flds],
                            f"}} lm_interfaces__msg__{n};", "#endif", ""])
    return out

def write_include_tree(root: pathlib.Path):
    d = root / "lm_interfaces/msg"; d.mkdir(parents=True, exist_ok=True)
    for n, h in headers().items(): (d / f"{snake(n)}.h").write_text(h)
    return root
