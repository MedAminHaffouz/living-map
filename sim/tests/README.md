# tests/

Contract and spec-violation tests for `core/runner.py`. No module-behavior tests yet.

- `test_contracts.py` — asserts that wiring violations (Writer subscribing to
  `executor/brief`, Executor publishing `writer/event`) raise `WiringError`, and that the
  committed `config/wiring.yaml` loads and validates cleanly.

| file | class | inputs | outputs | status |
|---|---|---|---|---|
| test_contracts.py | test_writer_cannot_listen_to_brief, test_executor_cannot_publish_writer_events, test_full_graph_valid | none (constructs `Module` subclasses in-process) | none (pytest assertions) | P1 done |

## Boundary

Tests import `core/` and `contracts/` only, plus `config/wiring.yaml` for the full-graph
test. No bus traffic crosses a zone boundary in this folder — it's whitebox validation
of the wiring rules, not an end-to-end sim run (that's `run.py`).

## Rules for contributors

1. New spec rules added to `core/zones.py`'s `ALLOWED` table should get a corresponding
   violation test here (e.g. the "no robot<->CP" and STRATEGY-broadcast-only rules are
   currently untested — TODO).
2. Tests may import `core/` and `contracts/` freely; avoid importing concrete
   `modules/*` classes except to construct minimal "Cheat" subclasses for violation
   tests, so tests stay fast and don't depend on sim behavior.
3. `test_full_graph_valid` must keep passing against the real `config/wiring.yaml` —
   treat a failure there as a wiring regression, not a test bug.
