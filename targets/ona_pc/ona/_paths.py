import sys, pathlib
R = pathlib.Path(__file__).resolve().parents[3]
sys.path[:0] = [str(R / "libs"), str(R / "contracts/generated/py")]
