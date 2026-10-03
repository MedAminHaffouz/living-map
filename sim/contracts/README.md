# contracts/

Single source of truth for every message type and topic that crosses a module boundary.
Nothing here executes logic — it's types + a static registry.

- `messages.py` — all `Header`/enum/dataclass message types. `BeaconPayload` is the one
  wire-format type; it must mirror `contracts/beacon.h` (TODO, not yet written) for the
  ESP32 firmware. Everything else is a pure Python message.
- `topics.py` — `TOPICS: {topic_name: (MessageType, owning_zone)}`. `core/runner.py`
  validates every module's declared `INPUTS`/`OUTPUTS` against this table at load time.

| file | class | inputs | outputs | status |
|---|---|---|---|---|
| messages.py | Header, Frame, EventType, Priority, Age, MissionState, Pose2D, SensorDetection, Event, TriageDecision, DropCommand, BeaconPayload, BeaconWrite, MissionLog, GeoEvent, Brief, BeaconObs, MissionStateMsg | n/a (types only) | n/a (types only) | P1 done |
| topics.py | `TOPICS` dict | n/a (static data) | n/a (static data) | P1 done |

## Boundary

`contracts/` has no inbound/outbound topics of its own — every other zone imports from
it. It is the shared vocabulary, not a participant in the bus.

## Rules for contributors

1. This is the **only** place message shapes are defined. If a module needs a new field,
   add it here first, then update `topics.py` if a new topic is needed.
2. Never remove or repurpose a field — add a new one with a default, so old messages
   (e.g. replayed logs) stay valid.
3. `BeaconPayload` fields must stay fixed-width-representable (uint8/uint16/uint32) since
   it is the actual RF wire format on hardware (P2).
4. Nothing in `contracts/` may import from `core/` or `modules/` — it sits below both.
