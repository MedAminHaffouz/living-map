# modules/writer/

The exploring robot: pose, sensing, fusion, triage, drop policy, beacon writing, and
mission log upload. All WRITER-zone logic lives here.

| file | class | inputs | outputs | status |
|---|---|---|---|---|
| sim_pose.py | SimPose | none (reads `sim.world.WORLD` directly) | `writer/pose` -> Pose2D | P1 done (sim stand-in); P2: real SLAM/EKF |
| sensor.py | SimScalarSensor | `writer/pose` | `writer/det` -> SensorDetection | P1 done for FIRE/GAS (S1/S3); S2/S4 not wired — TODO; P2: real sensor drivers |
| fusion.py | Fusion | `writer/pose`, `writer/det` | `writer/event` -> Event | P1 done; co-occurrence severity boost TODO |
| triage.py | Triage | `writer/event` | `writer/triage` -> TriageDecision | P1 done (fixed severity bins); P2: LinUCB bandit |
| drop_policy.py | DropPolicy | `writer/pose`, `writer/triage` | `writer/drop` -> DropCommand | P1 done (spacing+event only); "rssi" reason unimplemented — TODO |
| beacon_writer.py | BeaconWriter | `writer/drop`, `writer/event`, `writer/triage`, `mission/state` | `beacon/write` -> BeaconWrite, `ona/upload` -> MissionLog | P1 done |

## Pipeline (one tick, EXPLORE state)

`sim_pose` -> `writer/pose` -> {`sensor`, `fusion`, `drop_policy`}
`sensor` -> `writer/det` -> `fusion` -> `writer/event` -> {`triage`, `beacon_writer`}
`triage` -> `writer/triage` -> {`drop_policy`, `beacon_writer`}
`drop_policy` -> `writer/drop` -> `beacon_writer` -> `beacon/write` (to BEACON)

At `mission/state == UPLOAD`, `beacon_writer` emits `ona/upload` once — this is the
**only** Writer egress to outside the robot (per `core/zones.py`, WRITER -> ONA only;
no WRITER -> EXECUTOR link exists).

## Boundary

- Inbound: `writer/pose`/`writer/det`/`writer/event`/`writer/triage`/`writer/drop` are
  internal to this zone. `mission/state` is broadcast in from STRATEGY.
- Outbound: `beacon/write` crosses to BEACON. `ona/upload` crosses to ONA. Nothing
  crosses directly to EXECUTOR or CP — `core/zones.py` enforces this at load time.

## Rules for contributors

1. Import only `contracts/` and `core/` (plus `sim/` for the two sim-only drivers). Never
   import another `modules/*` package.
2. Keep `step()` deterministic given `params['seed']` — any new randomness needs its own
   seeded `random.Random`, not shared global state.
3. New sensor channels (S2 smoke, S4 person) are new `wiring.yaml` entries using
   `SimScalarSensor` (or a new class if the pipeline genuinely differs — S4 will), not
   edits to existing sensor params.
