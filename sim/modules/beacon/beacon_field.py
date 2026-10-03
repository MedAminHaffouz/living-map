"""Simulated beacon field: every deployed beacon's current state, broadcasting each tick.

Zone: BEACON.
Inputs: beacon/write -> BeaconWrite (from Writer), beacon/update -> BeaconWrite (from Executor overwrite).
Outputs: beacon/broadcast -> BeaconObs.
Active in: always.
Params read from config/wiring.yaml: none.
P1 status: keyed dict id -> latest BeaconWrite, kept only if the incoming version is >=
the stored one (version never decreases); every beacon is rebroadcast every tick with a
fixed rssi=-50.0.
P2 plan: this module disappears — each beacon is its own ESP32-C3 firmware instance;
rssi comes from the radio hardware, not a constant. beacon/write, beacon/update,
beacon/broadcast stay the same topics/types.
"""
from core.module import Module
from core.zones import Zone
from contracts.messages import BeaconObs

class BeaconField(Module):
    """All deployed beacons. Stores latest version per id, broadcasts each tick.
    P2: this is firmware on each ESP32-C3; RSSI comes from the radio, not a formula."""
    ZONE = Zone.BEACON; INPUTS = ("beacon/write", "beacon/update"); OUTPUTS = ("beacon/broadcast",)
    def __init__(self, name, params=None):
        """Track the latest BeaconWrite per beacon id."""
        super().__init__(name, params); self.store = {}
    def step(self, t, inbox):
        """Apply writes/updates (keeping only non-decreasing versions), then rebroadcast every beacon."""
        for w in inbox["beacon/write"] + inbox["beacon/update"]:
            cur = self.store.get(w.payload.id)
            if cur is None or w.payload.version >= cur.payload.version: self.store[w.payload.id] = w
        return {"beacon/broadcast": [BeaconObs(self.hdr(t), w.payload, rssi=-50.0) for w in self.store.values()]}
