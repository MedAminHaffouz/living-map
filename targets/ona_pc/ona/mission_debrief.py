"""Mission debrief: CP mission (JSON) -> BriefHeader + BriefStep frames -> radio -> Executor (at the entrance).
Mission JSON: {"mission_id": 1, "steps": [{"beacon_id": 3, "action": "EXTINGUISH", "event": "FIRE"}, ...]}"""
from . import _paths  # noqa
import time
import lm_msgs as M
from lm_core.link import encode

def frames(mission: dict) -> list[bytes]:
    mid, steps = mission["mission_id"], mission["steps"]
    out = [encode(M.BriefHeader.ID, M.BriefHeader(mid, len(steps), int(time.time())).pack())]
    for i, s in enumerate(steps):
        out.append(encode(M.BriefStep.ID, M.BriefStep(mid, i, s["beacon_id"], M.Action[s["action"]], M.EventType[s["event"]]).pack()))
    return out
