"""Transport-agnostic bus. InProcBus for sim; MqttBus/SerialBus later
implement the same 2 methods, so modules never change between sim and HW.

Zone: core/ (infrastructure, not a spec zone).
Inputs: none (library code, called by core/runner.py).
Outputs: none.
Active in: always.
Params read from config/wiring.yaml: none (bus is constructed by Runner, not wired as a module).
P1 status: InProcBus implemented — per-topic append-only log with one read cursor per
(topic, reader), so every subscriber replays from where it left off and history() gives
full replay for debugging/tests. Optional drop_fn supports fault injection.
P2 plan: add MqttBus/SerialBus implementing the same Bus protocol (publish/drain); modules
and the runner are unaffected since they only depend on the protocol.
TODOs:
    - TODO: implement MqttBus / SerialBus for hardware (P2).
"""
from collections import defaultdict
from typing import Protocol, Any

class Bus(Protocol):
    """Transport protocol every bus implementation (sim or HW) must satisfy."""
    def publish(self, topic: str, msg: Any) -> None:
        """Append msg to topic."""
        ...
    def drain(self, topic: str, reader: str) -> list[Any]:
        """Return and consume all messages on topic not yet seen by reader."""
        ...

class InProcBus:
    """In-process bus: per-topic log + per-(topic, reader) replay cursor."""
    def __init__(self, drop_fn=None):
        self._log = defaultdict(list)          # topic -> all msgs (replayable)
        self._cursor = defaultdict(int)        # (topic, reader) -> index
        self.drop_fn = drop_fn                 # fault injection hook: (topic,msg)->bool

    def publish(self, topic, msg):
        """Append msg to topic's log, unless drop_fn(topic, msg) says to drop it."""
        if self.drop_fn and self.drop_fn(topic, msg):
            return
        self._log[topic].append(msg)

    def drain(self, topic, reader):
        """Return messages on topic published since reader's last drain, advancing its cursor."""
        k = (topic, reader); i = self._cursor[k]
        out = self._log[topic][i:]; self._cursor[k] = len(self._log[topic])
        return out

    def history(self, topic):
        """Return the full, un-consumed log for topic (for inspection/tests)."""
        return list(self._log[topic])
