# sim/

Ground-truth world model for P1. Only the Writer's sim drivers
(`modules/writer/sim_pose.py`, `modules/writer/sensor.py`) are allowed to read it
directly (not over the bus — plain Python import).

- `world.py` — `World` class + `WORLD` singleton: fixed fire/gas sources, a fixed travel
  path, `pose_at(t)` and `field(srcs, x, y)` helpers.

| file | class | inputs | outputs | status |
|---|---|---|---|---|
| world.py | World (`WORLD` singleton) | none (in-memory, not wired) | none (read via Python import, not a topic) | P1 done (sim-only; disappears entirely on hardware) |

`faults.py` (fault-injection scenarios) is referenced in the root README as TODO and
does not exist yet.

## Boundary

`sim/` crosses no bus topic. It is a privileged, sim-only backdoor: `SimPose` and
`SimScalarSensor` (both zone WRITER) import `WORLD` directly to synthesize
`writer/pose` and `writer/det`. No other module may import `sim/`.

## Rules for contributors

1. Only `modules/writer/sim_pose.py` and `modules/writer/sensor.py` (or their P1
   successors) may import `sim.world`. If a new sim driver needs ground truth, it must
   live in `modules/writer/` and import from here — never the reverse.
2. `sim/` must not import from `modules/` or `core/`; it is the lowest layer, same rank
   as `contracts/`.
3. Nothing in `sim/` survives to P2 — do not let real hardware code depend on it.
