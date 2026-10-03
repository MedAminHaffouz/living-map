"""SLAM stand-in (P1): ground truth + odometry drift. Swap for EKF/slam_toolbox in P2.

Zone: WRITER.
Inputs: none (reads sim.world.WORLD ground truth directly; sim-only privilege).
Outputs: writer/pose -> Pose2D.
Active in: EXPLORE, RETURN.
Params read from config/wiring.yaml: seed (int, default 0), drift_sigma (m/tick std dev, default 0.0).
P1 status: takes ground-truth pose from WORLD.pose_at(t) and adds an accumulating
gaussian random walk (drift_sigma per axis) to emulate odometry drift; no actual SLAM.
P2 plan: replace with a real EKF or slam_toolbox node; writer/pose -> Pose2D stays the
same contract so downstream (sensor, fusion, drop_policy) needs no change.
"""
import random
from core.module import Module
from core.zones import Zone
from contracts.messages import Pose2D, Frame, MissionState as S
from sim.world import WORLD

class SimPose(Module):
    """Publishes drifting odometry pose derived from sim ground truth."""
    ZONE = Zone.WRITER; OUTPUTS = ("writer/pose",); ACTIVE_IN = (S.EXPLORE, S.RETURN)
    def __init__(self, name, params=None):
        """Seed the drift RNG and zero the accumulated drift offset (ex, ey)."""
        super().__init__(name, params); self.rng = random.Random(self.p.get("seed", 0)); self.ex = self.ey = 0.0
    def step(self, t, inbox):
        """Sample ground-truth pose at time t (s), add accumulated gaussian drift, publish Pose2D."""
        x, y, th = WORLD.pose_at(t)
        s = self.p.get("drift_sigma", 0.0)
        self.ex += self.rng.gauss(0, s); self.ey += self.rng.gauss(0, s)
        return {"writer/pose": [Pose2D(self.hdr(t, Frame.W), x + self.ex, y + self.ey, th)]}
