from __future__ import annotations
from dataclasses import dataclass
from typing import Any, ClassVar
from core.zones import Zone
from contracts.messages import Header, MissionState

class Module:
    """Contract: declare ZONE, INPUTS, OUTPUTS (topic names from contracts.topics).
    Implement step(t, inbox) -> {topic: [msgs]}. No I/O, no globals, no imports
    of other modules. Deterministic given params['seed'] -> replayable.

    Zone: core/ (base class, not itself a zone — subclasses set ZONE).
    Inputs: declared per-subclass via INPUTS (topic names checked against contracts/topics.py).
    Outputs: declared per-subclass via OUTPUTS.
    Active in: declared per-subclass via ACTIVE_IN; None means every mission state.
    Params read from config/wiring.yaml: none directly — params dict is passed through
    by core/runner.py from each module's `params:` block and stored as self.p.
    P1 status: base class is complete; used by every module in modules/.
    P2 plan: unchanged on hardware — step() stays the same interface regardless of transport.
    """
    ZONE: ClassVar[Zone]
    INPUTS: ClassVar[tuple[str, ...]] = ()
    OUTPUTS: ClassVar[tuple[str, ...]] = ()
    ACTIVE_IN: ClassVar[tuple[MissionState, ...] | None] = None   # None = always

    def __init__(self, name: str, params: dict | None = None):
        """Store the module's wiring name and its params dict (from wiring.yaml)."""
        self.name = name; self.p = params or {}; self._seq = 0

    def hdr(self, t, frame=None) -> Header:
        """Build a Header for a new outgoing message, auto-incrementing the per-module seq."""
        self._seq += 1
        return Header(stamp=t, source=self.name, frame=frame, seq=self._seq)

    def step(self, t: float, inbox: dict[str, list[Any]]) -> dict[str, list[Any]]:
        """Process one tick at mission time t (s): consume inbox, return {topic: [msgs]} to publish."""
        raise NotImplementedError
