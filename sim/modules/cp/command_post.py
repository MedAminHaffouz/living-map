"""Command Post: approves a mission plan from the situation picture the ONA carries to it.

Zone: CP.
Inputs: cp/situation -> GeoEvent (from ONA; the only CP inbound, no direct robot link).
Outputs: cp/plan -> Brief (to ONA, which relays it onward to the Executor).
Active in: always.
Params read from config/wiring.yaml: none.
P1 status: auto-approves every situation tick it receives events for: targets = all
event ids seen so far this tick, route and beacon_table left empty.
P2 plan: route should come from a real path planner (Dijkstra over the beacon
topology) and beacon_table should be populated; targets may involve human/operator
approval instead of auto-approve. cp/situation -> cp/plan contract unchanged.
TODOs:
    - TODO: route is always empty; no path planner implemented.
    - TODO: beacon_table is always empty; Brief never carries beacon payloads to the Executor.
"""
from core.module import Module
from core.zones import Zone
from contracts.messages import Brief

class CommandPost(Module):
    """Auto-approves: targets = all events (P1). Real route = Dijkstra in ONA brief."""
    ZONE = Zone.CP; INPUTS = ("cp/situation",); OUTPUTS = ("cp/plan",)
    def step(self, t, inbox):
        """Auto-approve a Brief targeting every GeoEvent id received this tick."""
        ev = inbox["cp/situation"]
        if not ev: return
        return {"cp/plan": [Brief(self.hdr(t), route=(), targets=tuple(g.event.id for g in ev), beacon_table=())]}
