"""Topic registry: name -> (message type, zone that may publish it).

Zone: contracts/ (shared by all zones; contracts/ is not itself a zone).
Inputs: none (static table).
Outputs: none (consumed by core/runner.py for load-time validation).
Active in: always (read at wiring load, not per-tick).
Params read from config/wiring.yaml: none.
P1 status: table is complete and matches every module's declared INPUTS/OUTPUTS in
config/wiring.yaml; runner.validate() checks both topic existence and publisher zone.
P2 plan: unchanged — the topic/type/zone table is transport-agnostic, so swapping Bus
impls (MQTT/serial) for hardware does not touch this file.
The runner refuses any wiring that disagrees with this table."""
from core.zones import Zone
from contracts.messages import *

TOPICS = {
    # Writer internal
    "writer/pose":        (Pose2D,          Zone.WRITER),
    "writer/det":         (SensorDetection, Zone.WRITER),
    "writer/event":       (Event,           Zone.WRITER),
    "writer/triage":      (TriageDecision,  Zone.WRITER),
    "writer/drop":        (DropCommand,     Zone.WRITER),
    # Writer -> beacons (physical radio write)
    "beacon/write":       (BeaconWrite,     Zone.WRITER),
    # Writer -> ONA (upload at dock) : the only Writer egress to outside
    "ona/upload":         (MissionLog,      Zone.WRITER),
    # Beacons -> anyone inside in radio range
    "beacon/broadcast":   (BeaconObs,       Zone.BEACON),
    # ONA internal + egress
    "ona/geo_event":      (GeoEvent,        Zone.ONA),
    "cp/situation":       (GeoEvent,        Zone.ONA),
    "cp/plan":            (Brief,           Zone.CP),
    "executor/brief":     (Brief,           Zone.ONA),
    # Executor -> beacons (overwrite)
    "beacon/update":      (BeaconWrite,     Zone.EXECUTOR),
    # Strategy
    "mission/state":      (MissionStateMsg, Zone.STRATEGY),
}
