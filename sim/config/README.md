# config/

The module graph, as data. `core/runner.py:load()` reads this to instantiate every
module; `validate()` then checks the graph against `contracts/topics.py` and
`core/zones.py` before any tick runs.

- `wiring.yaml` — top-level `dt` (s) and `duration_s` (s), plus a `modules:` list of
  `{name, impl: <dotted.path.ClassName>, params: {...}}`. Topics are **not** listed here
  — they're implicit from each module's `INPUTS`/`OUTPUTS` class attributes.

| file | class | inputs | outputs | status |
|---|---|---|---|---|
| wiring.yaml | n/a (data) | n/a | n/a | P1 done — wires fsm, pose, s1_fire, s3_gas, fusion, triage, drop, bwriter, beacons, ona_tr, cp, ona_brief, executor |

`scenarios/*.yaml` (failure-injection cases, referenced in the root README) does not
exist yet — TODO.

## Boundary

`config/` crosses no bus topic itself; it's the declarative input to `core/runner.py`.
Every topic that actually crosses a zone boundary is declared by the modules this file
wires, per `contracts/topics.py`.

## Rules for contributors

1. Sim vs. hardware is a `wiring.yaml` edit, not a code change: swap a module's `impl:`
   (e.g. `modules.writer.sensor.SimScalarSensor` -> a real driver class) and keep its
   `INPUTS`/`OUTPUTS` topic names identical.
2. Every module listed here must exist as a class with `ZONE`, `INPUTS`, `OUTPUTS` set;
   `validate()` will reject the whole graph otherwise.
3. S2 (smoke) and S4 (person) sensors are not yet wired — adding them means adding a
   `modules:` entry here, not changing `sensor.py`'s logic (see modules/writer/README.md).
