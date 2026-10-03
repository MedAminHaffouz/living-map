"""Generic sensor node: read -> filter -> feature -> detect. S1..S4 = params, not new classes,
unless a sensor genuinely needs its own pipeline (S4 people/CSI will).

Zone: WRITER.
Inputs: writer/pose -> Pose2D.
Outputs: writer/det -> SensorDetection.
Active in: EXPLORE.
Params read from config/wiring.yaml: type (EventType name, e.g. "FIRE"/"GAS", required),
noise (gaussian std dev added to the field reading, default 0.05), seed (int, default 1),
ema (EMA smoothing factor alpha, default 0.3).
P1 status: reads a scalar field (WORLD.fires or WORLD.gas) at the current pose plus
gaussian noise, EMA-smooths it, and emits the smoothed value as both p and value. One
instance per sensor channel (wiring.yaml wires s1_fire, s3_gas); S2/S4 are TODO.
P2 plan: swap for a real driver (e.g. thermal camera for FIRE, MQ-7 for GAS) behind the
same writer/pose -> writer/det contract; fusion.py and everything downstream is unchanged.
TODOs:
    - TODO: S2 (smoke) and S4 (person/CSI) sensors are not wired in config/wiring.yaml yet.
"""
import random
from core.module import Module
from core.zones import Zone
from contracts.messages import SensorDetection, EventType, MissionState as S
from sim.world import WORLD

class SimScalarSensor(Module):
    """Simulated scalar sensor: ground-truth field sample + noise, EMA-smoothed."""
    ZONE = Zone.WRITER; INPUTS = ("writer/pose",); OUTPUTS = ("writer/det",); ACTIVE_IN = (S.EXPLORE,)
    def __init__(self, name, params=None):
        """Resolve the EventType this instance senses and seed its noise RNG."""
        super().__init__(name, params)
        self.type = EventType[self.p["type"]]; self.rng = random.Random(self.p.get("seed", 1))
        self.ema = 0.0; self.a = self.p.get("ema", 0.3)
    def read(self, x, y):
        """Sample the ground-truth field for this sensor's EventType at (x, y) (m), plus gaussian noise."""
        src = {EventType.FIRE: WORLD.fires, EventType.GAS: WORLD.gas}[self.type]
        return WORLD.field(src, x, y) + self.rng.gauss(0, self.p.get("noise", 0.05))
    def step(self, t, inbox):
        """EMA-smooth a new field reading at the latest pose and publish it as a SensorDetection."""
        if not inbox["writer/pose"]: return
        pose = inbox["writer/pose"][-1]
        self.ema = self.a * self.read(pose.x, pose.y) + (1 - self.a) * self.ema
        p = min(max(self.ema, 0.0), 1.0)
        return {"writer/det": [SensorDetection(self.hdr(t), self.type, p, conf=0.9, value=self.ema)]}
