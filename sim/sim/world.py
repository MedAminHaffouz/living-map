"""Ground truth. Only sensor/pose DRIVERS may read it (sim_* modules).

Zone: SIM (ground truth; not a spec zone, feeds WRITER sim drivers per core/zones.py).
Inputs: none (standalone in-memory model).
Outputs: none (consumed directly by modules.writer.sim_pose.SimPose and
modules.writer.sensor.SimScalarSensor via Python import, not a bus topic).
Active in: always (plain object, not a Module; no ACTIVE_IN).
Params read from config/wiring.yaml: none — WORLD is a module-level singleton, not wired.
P1 status: fixed fire/gas sources (x, y, radius in frame W) and a fixed polyline path;
pose_at(t) walks the path at constant speed, field() returns a gaussian-falloff
intensity in [0,1] from the nearest source.
P2 plan: disappears entirely — on hardware there is no ground truth, only real sensor
and odometry readings. Sim-only module; never referenced outside sim_pose.py/sensor.py.
"""
import math, random

class World:
    """In-memory ground-truth model: fire/gas sources, a travel path, and a gaussian field."""
    def __init__(self, seed=0):
        """Seed the RNG (reserved for future use) and fix the sources/path for this world."""
        self.rng = random.Random(seed)
        self.fires = [(6.0, 2.0, 1.5)]            # x, y, radius (m, frame W)
        self.gas = [(12.0, -1.0, 2.0)]             # x, y, radius (m, frame W)
        self.path = [(0, 0), (4, 0), (8, 2), (12, 0), (14, -2)]   # m, frame W
    def pose_at(self, t, speed=0.5):
        """Return (x, y, theta) (m, m, rad) at mission time t (s), walking the fixed path at speed (m/s)."""
        d = t * speed
        for (x0, y0), (x1, y1) in zip(self.path, self.path[1:]):
            L = math.hypot(x1 - x0, y1 - y0)
            if d <= L:
                a = d / L; return x0 + a*(x1-x0), y0 + a*(y1-y0), math.atan2(y1-y0, x1-x0)
            d -= L
        x, y = self.path[-1]; return x, y, 0.0
    def field(self, srcs, x, y):
        """Return the max gaussian-falloff intensity in [0,1] from a list of (x, y, radius) sources at (x, y)."""
        return max((math.exp(-((x-a)**2 + (y-b)**2) / r**2) for a, b, r in srcs), default=0.0)

WORLD = World()
