"""Generated wire dataclass <-> ROS msg, field-by-field (names are identical by construction)."""
from dataclasses import fields
def to_ros(dc, RosT):
    m = RosT()
    for f in fields(dc): setattr(m, f.name, getattr(dc, f.name))
    return m
def from_ros(m, DcT):
    return DcT(**{f.name: getattr(m, f.name) for f in fields(DcT)})
