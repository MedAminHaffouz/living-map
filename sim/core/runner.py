"""Builds the module graph from config/wiring.yaml, validates every contract
at wire-up time (fail fast), then ticks deterministically.

Zone: core/ (infrastructure, not a spec zone; enforces zone rules for all other zones).
Inputs: none directly — reads config/wiring.yaml and drains every topic each tick on
behalf of the wired modules.
Outputs: none directly — publishes whatever each module's step() returns.
Active in: always.
Params read from config/wiring.yaml: top-level `dt` (s) and `duration_s` (s), plus each
module's own `params:` block (passed through, not read here).
P1 status: load() builds modules from `impl:` dotted paths; validate() checks topic
existence, publisher-zone ownership (contracts/topics.py), and subscriber-zone edges
(core/zones.py ALLOWED) before any tick runs; Runner.tick()/run() drive the sim loop with
an InProcBus by default.
P2 plan: unchanged — load()/validate() are transport-agnostic; only the `impl:` targets
and the Bus passed to Runner change for hardware.
TODOs:
    - TODO: config/scenarios/*.yaml for fault-injection runs (referenced by root README, not yet present).
"""
import importlib, yaml
from contracts.topics import TOPICS
from contracts.messages import MissionState
from core.zones import ALLOWED
from core.bus import InProcBus

class WiringError(Exception):
    """Raised when wiring.yaml or a module's declared topics violate the spec."""
    ...

def load(path):
    """Load config/wiring.yaml, instantiate each module's `impl:` class with its params; return (mods, cfg)."""
    cfg = yaml.safe_load(open(path))
    mods = []
    for m in cfg["modules"]:
        mod_path, cls = m["impl"].rsplit(".", 1)
        C = getattr(importlib.import_module(mod_path), cls)
        mods.append(C(m["name"], m.get("params", {})))
    return mods, cfg

def validate(mods):
    """Check every module's OUTPUTS/INPUTS against contracts/topics.py and core/zones.py; raise WiringError on violation."""
    pubs = {}
    for m in mods:
        for t in m.OUTPUTS:
            if t not in TOPICS: raise WiringError(f"{m.name}: unknown topic {t}")
            _, zone = TOPICS[t]
            if zone != m.ZONE and m.ZONE.value != "SIM":
                raise WiringError(f"{m.name} ({m.ZONE.value}) may not publish {t} (owned by {zone.value})")
            pubs.setdefault(t, []).append(m)
    for m in mods:
        for t in m.INPUTS:
            if t not in TOPICS: raise WiringError(f"{m.name}: unknown topic {t}")
            _, pz = TOPICS[t]
            if m.ZONE not in ALLOWED[pz]:
                raise WiringError(f"spec violation: {pz.value} -> {m.ZONE.value} via {t} ({m.name})")

class Runner:
    """Owns the bus and mission state; ticks every module once per dt until DONE/ABORT."""
    def __init__(self, mods, dt=0.1, bus=None):
        """Validate mods, then hold them plus a Bus (InProcBus by default) and the starting mission state."""
        validate(mods)
        self.mods, self.dt = mods, dt   # dt: s
        self.bus = bus or InProcBus()
        self.state = MissionState.ENTER

    def tick(self, t):
        """Advance mission state from mission/state, then step every module active in that state at time t (s)."""
        for m in self.mods:
            for s in self.bus.drain("mission/state", m.name + "#fsm"):
                self.state = s.state
            if m.ACTIVE_IN is not None and self.state not in m.ACTIVE_IN:
                continue   # inactive: inputs stay queued (never silently dropped)
            inbox = {tp: self.bus.drain(tp, m.name) for tp in m.INPUTS}
            out = m.step(t, inbox) or {}
            for tp, msgs in out.items():
                if tp not in m.OUTPUTS: raise WiringError(f"{m.name} published undeclared {tp}")
                T, _ = TOPICS[tp]
                for msg in msgs:
                    if not isinstance(msg, T):
                        raise WiringError(f"{m.name}: {tp} expects {T.__name__}, got {type(msg).__name__}")
                    self.bus.publish(tp, msg)

    def run(self, T):
        """Tick every dt (s) from 0 to T (s) or until mission state is DONE/ABORT; return the bus."""
        t = 0.0
        while t < T and self.state not in (MissionState.DONE, MissionState.ABORT):
            self.tick(t); t += self.dt
        return self.bus
