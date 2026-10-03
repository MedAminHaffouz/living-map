"""Mission FSM: the single source of mission/state, driving every module's ACTIVE_IN gate.

Zone: STRATEGY.
Inputs: ona/upload -> MissionLog, executor/brief -> Brief, beacon/update -> BeaconWrite
    (observed only, to detect that a phase's expected event has happened; never relayed).
Outputs: mission/state -> MissionStateMsg.
Active in: always (FSM itself has no ACTIVE_IN gate).
Params read from config/wiring.yaml: explore_s (s, default 40), execute_s (s, default 5).
P1 status: single linear FSM (ENTER -> EXPLORE -> RETURN -> UPLOAD -> BRIEF -> EXECUTE ->
DONE) driven by elapsed time plus latched "have I seen this topic fire" flags; no ABORT
transition is ever emitted.
P2 plan: splits into one local FSM per robot (Writer, Executor) plus one in the ONA;
the states/transitions here are the contract each side must honour across that split.
TODOs:
    - TODO: no path currently reaches MissionState.ABORT; add failure-triggered transitions.
"""
from core.module import Module
from core.zones import Zone
from contracts.messages import MissionState as S, MissionStateMsg

class MissionFSM(Module):
    """Orchestrates phases only. Observes, never relays data.
    On HW this splits into one local FSM per robot + ONA; transitions below
    are the contract each side must honour."""
    ZONE = Zone.STRATEGY
    INPUTS = ("ona/upload", "executor/brief", "beacon/update")
    OUTPUTS = ("mission/state",)

    def __init__(self, name, params=None):
        """Start with no state (step() will emit ENTER on the first tick)."""
        super().__init__(name, params); self.s = None; self.t0 = 0; self.seen = set()
    def go(self, t, s, why=""):
        """Transition to state s at time t (s), resetting the phase clock; return the mission/state message."""
        self.s, self.t0 = s, t
        return {"mission/state": [MissionStateMsg(self.hdr(t), s, why)]}
    def step(self, t, inbox):
        """Advance the FSM one tick: latch which input topics fired, then apply the next due transition."""
        self.seen |= {k for k, v in inbox.items() if v}   # latch: downstream may fire in the same tick
        if self.s is None: return self.go(t, S.ENTER, "start")
        if self.s == S.ENTER and t - self.t0 > 0.5: return self.go(t, S.EXPLORE)
        if self.s == S.EXPLORE and t - self.t0 > self.p.get("explore_s", 40): return self.go(t, S.RETURN, "budget")
        if self.s == S.RETURN and t - self.t0 > 1: return self.go(t, S.UPLOAD)
        if self.s == S.UPLOAD and "ona/upload" in self.seen: return self.go(t, S.BRIEF)
        if self.s == S.BRIEF and "executor/brief" in self.seen: return self.go(t, S.EXECUTE)
        if self.s == S.EXECUTE and t - self.t0 > self.p.get("execute_s", 5): return self.go(t, S.DONE)
