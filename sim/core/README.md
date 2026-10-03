# core/

Infrastructure every module runs on: the `Module` base contract, the transport-agnostic
`Bus`, the `Zone` spec rules, and the `Runner` that wires + validates + ticks the graph.
No message types live here — those are in `contracts/`.

- `module.py` — `Module` base class. Subclasses declare `ZONE`, `INPUTS`, `OUTPUTS`,
  optional `ACTIVE_IN`, and implement `step(t, inbox) -> {topic: [msgs]}`.
- `bus.py` — `Bus` protocol (`publish`, `drain`) + `InProcBus` (sim). `MqttBus`/`SerialBus`
  (P2) implement the same protocol so modules never change between sim and HW.
- `zones.py` — `Zone` enum + `ALLOWED` publisher-zone -> subscriber-zone edge table,
  encoding the spec (ONA gateway, no Writer<->Executor, no robot<->CP, STRATEGY
  broadcast-only).
- `runner.py` — `load()` builds modules from `config/wiring.yaml`; `validate()` checks
  every topic/zone rule before any tick runs; `Runner` ticks deterministically.

| file | class | inputs | outputs | status |
|---|---|---|---|---|
| module.py | Module | per-subclass `INPUTS` | per-subclass `OUTPUTS` | P1 done |
| bus.py | Bus (protocol), InProcBus | n/a | n/a | P1 done (sim); MqttBus/SerialBus = P2 TODO |
| zones.py | Zone | n/a (static data) | n/a (static data) | P1 done |
| runner.py | WiringError, Runner, `load()`, `validate()` | reads `config/wiring.yaml` | drives every topic via Bus | P1 done |

## Boundary

`core/` has no topics of its own. It is loaded by `run.py` and imported by every module
and by `contracts/topics.py`/`zones.py` consumers. All inter-zone traffic is topics
declared by `modules/*` and validated here, not traffic `core/` originates.

## Rules for contributors

1. `core/` may import from `contracts/` only, never from `modules/`.
2. `Module.step()` must stay pure given `(t, inbox, self.p)` — no I/O, no globals, no
   imports of other modules — so sim runs are replayable.
3. Any new transport (`MqttBus`, `SerialBus`) must implement the `Bus` protocol exactly;
   do not change `Module`/`Runner` to accommodate a transport quirk.
4. Zone edge changes in `zones.py` are spec changes — they encode the "no Writer<->
   Executor, no robot<->CP, all traffic through ONA" rules; don't loosen them casually.
