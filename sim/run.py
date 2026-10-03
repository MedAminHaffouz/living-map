"""Entry point: load config/wiring.yaml, run the sim for duration_s, print a topic summary.

Zone: none (top-level script; orchestrates core/runner.py).
Inputs: CLI arg argv[1] = wiring config path (default config/wiring.yaml).
Outputs: stdout only — per-topic message counts and the last message on key topics.
Active in: n/a (not a Module).
Params read from config/wiring.yaml: dt (s) and duration_s (s), passed to Runner.run().
P1 status: runs the full sim graph once and prints a fixed list of topics for a quick
smoke check (mission/state, writer/event, beacon/write, cp/situation, executor/brief,
beacon/update).
P2 plan: unchanged — swapping `impl:` entries in wiring.yaml for hardware drivers does
not change this script.
"""
import sys, yaml
from core.runner import load, Runner
mods, cfg = load(sys.argv[1] if len(sys.argv) > 1 else "config/wiring.yaml")
bus = Runner(mods, dt=cfg["dt"]).run(cfg["duration_s"])
for tp in ["mission/state", "writer/event", "beacon/write", "cp/situation", "executor/brief", "beacon/update"]:
    h = bus.history(tp); print(f"{tp:18s} {len(h):3d}", "|", h[-1] if h and tp != "beacon/write" else "")
