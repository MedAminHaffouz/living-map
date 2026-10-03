"""Zone enum and the allowed publisher-zone -> subscriber-zone edge table.

Zone: core/ (infrastructure; defines what a "zone" is for every other zone).
Inputs: none (static table).
Outputs: none; consumed by core/runner.py's validate().
Active in: always (checked once at wiring load).
Params read from config/wiring.yaml: none.
P1 status: complete — encodes the full spec (ONA gateway, no Writer<->Executor,
no robot<->CP, STRATEGY broadcast-only).
P2 plan: unchanged on hardware — zone edges are a logical/spec property, not a transport one.
"""
from enum import Enum

class Zone(str, Enum):
    """One box in the architecture diagram; every Module declares exactly one."""
    WRITER = "WRITER"; BEACON = "BEACON"; ONA = "ONA"; CP = "CP"
    EXECUTOR = "EXECUTOR"; STRATEGY = "STRATEGY"; SIM = "SIM"

# Allowed publisher-zone -> subscriber-zone edges. Encodes the spec:
# all inside<->outside through ONA, no Writer<->Executor, no robot<->CP.
# STRATEGY may observe everything but only publishes mission/state (no data relay).
_ALLOWED = {
    Zone.WRITER:   {Zone.WRITER, Zone.BEACON, Zone.ONA},
    Zone.BEACON:   {Zone.WRITER, Zone.EXECUTOR, Zone.ONA},   # ONA hears B0 only (gateway)
    Zone.ONA:      {Zone.ONA, Zone.CP, Zone.EXECUTOR},       # Executor gets brief at dock
    Zone.CP:       {Zone.ONA},
    Zone.EXECUTOR: {Zone.EXECUTOR, Zone.BEACON},
    Zone.STRATEGY: set(Zone),                                # state is broadcast to all
    Zone.SIM:      set(Zone),                                # ground truth feeds drivers
}

ALLOWED = {k: v | {Zone.STRATEGY} for k, v in _ALLOWED.items()}
