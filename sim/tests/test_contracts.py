"""Spec-violation + full-graph wiring tests for core/runner.py's validate().

Zone: tests/ (not a spec zone).
Inputs: none (constructs Module subclasses in-process; no bus/topics used).
Outputs: none (pytest assertions only).
Active in: n/a.
Params read from config/wiring.yaml: test_full_graph_valid loads the real file to check
it validates; no params are read by name.
P1 status: covers the two spec-violation cases named in core/zones.py (no
Writer<->Executor, no bad zone ownership on publish) plus a full-graph load+validate
smoke test against the committed config/wiring.yaml.
P2 plan: unchanged — these are pure wiring/contract tests, independent of sim vs. hardware.
TODOs:
    - TODO: no test yet for the "no robot<->CP" rule or the STRATEGY broadcast-only rule.
"""
import pytest
from core.runner import validate, WiringError
from core.module import Module
from core.zones import Zone

def test_writer_cannot_listen_to_brief():
    """WRITER subscribing to executor/brief (owned by ONA, not reachable by WRITER) must fail validation."""
    class Cheat(Module):
        ZONE = Zone.WRITER; INPUTS = ("executor/brief",)
    with pytest.raises(WiringError, match="spec violation"):
        validate([Cheat("cheat")])

def test_executor_cannot_publish_writer_events():
    """EXECUTOR publishing writer/event (owned by WRITER) must fail validation."""
    class Cheat(Module):
        ZONE = Zone.EXECUTOR; OUTPUTS = ("writer/event",)
    with pytest.raises(WiringError):
        validate([Cheat("cheat")])

def test_full_graph_valid():
    """The committed config/wiring.yaml must load and validate without error."""
    from core.runner import load
    mods, _ = load("config/wiring.yaml"); validate(mods)
