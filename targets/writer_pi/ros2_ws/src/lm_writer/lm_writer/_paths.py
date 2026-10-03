"""Make repo-level libs/ and generated contracts importable from ROS nodes (no pip install needed).
Set LM_REPO=/path/to/living-map, or it is found by walking up from this file in a --symlink-install build."""
import os, sys, pathlib
def setup():
    root = os.environ.get("LM_REPO")
    if not root:
        for p in pathlib.Path(__file__).resolve().parents:
            if (p / "contracts" / "schema.yaml").exists(): root = str(p); break
    if not root: raise RuntimeError("set LM_REPO to the living-map repo root")
    for sub in ("libs", "contracts/generated/py"):
        if os.path.join(root, sub) not in sys.path: sys.path.insert(0, os.path.join(root, sub))
setup()
