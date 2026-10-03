"""Executor: navigates by beacon, trusts by age, overwrites stale beacons.

Zone: EXECUTOR.
Inputs: executor/brief -> Brief (from ONA at dock), beacon/broadcast -> BeaconObs (from BEACON).
Outputs: beacon/update -> BeaconWrite (overwrite, version+1; the only Executor egress).
Active in: BRIEF, EXECUTE.
Params read from config/wiring.yaml: fresh_s (s, default 30), stale_s (s, default 120).
P1 status: latches the latest Brief; for each not-yet-visited beacon heard, classifies
its age from (t - when_s) vs fresh_s/stale_s, and if STALE (or older) re-writes it with
version+1 and when_s = now as a "verify stub". AGING beacons are neither followed nor
overwritten explicitly (no action beyond classification) and "contradicted" (vs. STALE)
is not distinguished — both paths call it stale-overwrite.
P2 plan: FRESH beacons should actively drive navigation ("follow"); a real verify step
(sensor re-check at the beacon's claimed event) should run before an AGING/STALE beacon
is overwritten, instead of overwriting on age alone. Contract (beacon/update ->
BeaconWrite) unchanged.
TODOs:
    - TODO: no navigation/following logic for FRESH beacons — Executor only classifies and overwrites.
    - TODO: "contradicted" vs. "stale" (per module docstring) is not actually distinguished in age().
"""
from dataclasses import replace
from core.module import Module
from core.zones import Zone
from contracts.messages import Age, BeaconWrite, MissionState as S

class Executor(Module):
    """Trust by age: fresh -> follow, stale -> verify, contradicted -> overwrite (version+1)."""
    ZONE = Zone.EXECUTOR; INPUTS = ("executor/brief", "beacon/broadcast"); OUTPUTS = ("beacon/update",)
    ACTIVE_IN = (S.BRIEF, S.EXECUTE)
    def __init__(self, name, params=None):
        """Track the latest Brief and the set of already-processed beacon ids."""
        super().__init__(name, params); self.brief = None; self.visited = set()
    def age(self, t, pl):
        """Classify a BeaconPayload's trust age from elapsed time (t - when_s, s) vs fresh_s/stale_s."""
        a = t - pl.when_s; f, s = self.p.get("fresh_s", 30), self.p.get("stale_s", 120)
        return Age.FRESH if a < f else Age.AGING if a < s else Age.STALE
    def step(self, t, inbox):
        """Latch the latest Brief, then overwrite (version+1) any newly-heard STALE-or-older beacon."""
        if inbox["executor/brief"]: self.brief = inbox["executor/brief"][-1]
        if not self.brief: return
        out = []
        for obs in inbox["beacon/broadcast"]:
            if obs.payload.id in self.visited: continue
            self.visited.add(obs.payload.id)
            if self.age(t, obs.payload) >= Age.STALE:   # verify stub: re-stamp after own check
                out.append(BeaconWrite(self.hdr(t), replace(obs.payload, version=obs.payload.version + 1, when_s=int(t)), (0.0, 0.0)))
        return {"beacon/update": out}
