"""Single source of truth for every message crossing a module boundary.

Zone: contracts/ (shared by all zones; contracts/ is not itself a zone).
Rule: modules import ONLY from contracts/ and core/, never from each other.
P1 status: all dataclasses below are implemented, frozen, and used end-to-end by sim.
P2 plan: BeaconPayload is the only wire-format type; it must mirror contracts/beacon.h
(TODO) field-for-field for the ESP32 firmware. Everything else stays in-process/Python
and travels over whatever Bus impl (MQTT/serial) P2 wires in.
TODOs:
    - TODO: write contracts/beacon.h mirroring BeaconPayload (uint8/uint16/uint32 widths).
"""
from __future__ import annotations
from dataclasses import dataclass, field
from enum import Enum, IntEnum


class Frame(str, Enum):
    """Coordinate frame tag carried on Header; translation code asserts on this."""
    W = "W"          # Writer odom frame, origin = B0 (entrance)
    ENU = "ENU"      # local east-north-up at entrance
    WGS84 = "WGS84"  # lat/lon


class EventType(IntEnum):
    """Sensed/detected phenomenon class, shared by sensors, events, and beacon payloads."""
    FIRE = 1; SMOKE = 2; GAS = 3; PERSON = 4; JUNCTION = 5


class Priority(IntEnum):          # triage bandit arms
    """Triage output tag, ignore..immediate; also the triage policy's action/arm space."""
    IGNORE = 0; LOW = 1; NORMAL = 2; HIGH = 3; IMMEDIATE = 4


class Age(IntEnum):
    """Beacon trust tier derived from time since write; FRESH is most trusted."""
    FRESH = 0; AGING = 1; STALE = 2; SUSPECT = 3


class MissionState(str, Enum):
    """Mission FSM phase; published on mission/state and gates each module's ACTIVE_IN."""
    ENTER = "ENTER"; EXPLORE = "EXPLORE"; RETURN = "RETURN"; UPLOAD = "UPLOAD"
    TRANSLATE = "TRANSLATE"; BRIEF = "BRIEF"; EXECUTE = "EXECUTE"; DONE = "DONE"; ABORT = "ABORT"


@dataclass(frozen=True)
class Header:
    """Common envelope on every message: mission clock, origin module, frame, sequence."""
    stamp: float            # mission clock, s; synced at B0
    source: str             # publishing module name
    frame: Frame | None = None
    seq: int = 0


@dataclass(frozen=True)
class Pose2D:
    """Writer pose estimate, frame W (or ENU after translation)."""
    header: Header
    x: float; y: float          # m
    theta: float                 # rad
    cov: tuple[float, float, float] = (0.0, 0.0, 0.0)   # diag var x,y,theta (m^2, m^2, rad^2)


@dataclass(frozen=True)
class SensorDetection:        # output of every S1..S4 node, same shape
    """Raw per-tick reading from one sensor channel, before fusion."""
    header: Header
    type: EventType
    p: float                  # detection probability, [0,1]
    conf: float               # sensor self-confidence (health, warm-up, noise), [0,1]
    value: float = 0.0        # raw feature: degC, ppm, variance... (unit depends on sensor)


@dataclass(frozen=True)
class Event:                  # fusion output
    """Confirmed, deduplicated detection anchored to a Writer pose. Frame must be W."""
    header: Header            # frame must be W
    id: int
    type: EventType
    severity: float           # [0,1]
    conf: float               # [0,1]
    x: float; y: float          # m, frame W
    co_events: tuple[EventType, ...] = ()


@dataclass(frozen=True)
class TriageDecision:
    """Priority tag assigned to one Event."""
    header: Header
    event_id: int
    priority: Priority


@dataclass(frozen=True)
class DropCommand:            # drop policy -> beacon writer
    """Instruction to place a beacon at (x, y)."""
    header: Header
    x: float; y: float          # m, frame W
    reason: str               # "spacing" | "rssi" | "junction" | "event"


@dataclass(frozen=True)
class BeaconPayload:          # WIRE FORMAT — mirrored in contracts/beacon.h
    """Fixed-width beacon record; this is the actual RF wire format on hardware."""
    id: int                   # uint8
    what: EventType           # uint8
    prio: Priority            # uint8
    dir_deg: int              # uint16, bearing to prev beacon, deg
    dist_cm: int              # uint16, distance to prev beacon, cm
    when_s: int               # uint32, mission clock at write, s
    version: int              # uint8, ++ on overwrite
    prev_id: int              # uint8, topology edge


@dataclass(frozen=True)
class BeaconWrite:
    """Command to write (or overwrite) a beacon's payload, plus the true drop pose for the log."""
    header: Header
    payload: BeaconPayload
    pose_W: tuple[float, float]   # true drop pose (m, m), frame W; kept in mission log only


@dataclass(frozen=True)
class MissionLog:             # Writer -> ONA at exit
    """Writer's full mission record, uploaded to the ONA once, at the dock."""
    header: Header
    events: tuple[Event, ...]
    beacons: tuple[BeaconWrite, ...]
    grid_png: bytes = b""


@dataclass(frozen=True)
class GeoEvent:
    """Event translated to world coordinates, with its current trust age."""
    header: Header            # frame WGS84
    event: Event
    lat: float; lon: float      # deg
    age: Age


@dataclass(frozen=True)
class Brief:                  # ONA -> Executor
    """Mission plan handed to the Executor before it re-enters: route, targets, beacon table."""
    header: Header
    route: tuple[int, ...]    # beacon ids in order
    targets: tuple[int, ...]  # event ids to act on
    beacon_table: tuple[BeaconPayload, ...]


@dataclass(frozen=True)
class BeaconObs:              # what Executor hears
    """One beacon as heard over radio by the Executor, with signal strength."""
    header: Header
    payload: BeaconPayload
    rssi: float                # dBm


@dataclass(frozen=True)
class MissionStateMsg:
    """Mission FSM phase broadcast, with an optional human-readable transition reason."""
    header: Header
    state: MissionState
    reason: str = ""
