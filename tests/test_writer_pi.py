"""Writer Pi ROS 2 package, checked without ROS: every node compiles, setup.py / files / launch agree,
calibration ops match the schema, and nothing references the removed serial bridges."""
import ast, pathlib, py_compile, re, yaml
R = pathlib.Path(__file__).resolve().parents[1]
WS = R / "targets/writer_pi/ros2_ws/src"
PKG = WS / "lm_writer/lm_writer"

def setup_nodes():
    tree = ast.parse((WS / "lm_writer/setup.py").read_text())
    return next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign) and n.targets[0].id == "NODES")

def test_every_node_compiles(tmp_path):
    for f in [*PKG.glob("*.py"), WS / "lm_bringup/launch/writer.launch.py"]:
        py_compile.compile(str(f), cfile=str(tmp_path / (f.name + "c")), doraise=True)

def test_setup_files_launch_agree():
    nodes = setup_nodes()
    for n in nodes: assert "def main(" in (PKG / f"{n}.py").read_text(), n
    launched = re.findall(r'lm\("(\w+)"\)', (WS / "lm_bringup/launch/writer.launch.py").read_text())
    assert set(launched) <= set(nodes) and "stm_adapter" in launched
    assert set(yaml.safe_load((WS / "lm_bringup/config/writer.yaml").read_text())) <= set(nodes)

def test_calibration_ops_match_schema():
    schema = yaml.safe_load((R / "contracts/schema.yaml").read_text())["enums"]["CalibOp"]
    ops = ast.literal_eval(re.search(r"CALIB_OPS = (\[.*?\])", (PKG / "map_processing.py").read_text()).group(1))
    assert ops == [schema["ENCODER_RESET"], schema["ZERO_BASELINE"], schema["IMU_BIAS"]]

def test_no_serial_bridges_left():
    gone = re.compile(r"stm_bridge|radio_bridge|_conv\b|_paths\b|import serial|pyserial|python3-serial")
    for f in [*PKG.glob("*.py"), *WS.glob("*/package.xml"), WS / "lm_writer/setup.py", *[f for f in (WS / "lm_bringup").rglob("*") if f.suffix in (".py", ".xml", ".yaml", ".txt")]]:
        assert not gone.search(f.read_text()), f
