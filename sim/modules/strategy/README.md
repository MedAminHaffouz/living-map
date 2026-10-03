# modules/strategy/

The mission FSM: the single source of `mission/state`. Observes other zones' topics
only to detect phase-completion signals; never relays data between zones.

| file | class | inputs | outputs | status |
|---|---|---|---|---|
| mission_fsm.py | MissionFSM | `ona/upload`, `executor/brief`, `beacon/update` (observed only) | `mission/state` -> MissionStateMsg | P1 done (linear ENTER->EXPLORE->RETURN->UPLOAD->BRIEF->EXECUTE->DONE); no ABORT path — TODO |

## Boundary

Per `core/zones.py`, STRATEGY may **observe** every zone's topics (`ALLOWED[Zone.STRATEGY]
== set(Zone)`), but its only output, `mission/state`, is broadcast to all zones
(`STRATEGY` is added to every zone's allowed-publisher set in `ALLOWED`). This is the
"observes everything, relays nothing" rule — `MissionFSM.step()` must never construct
or forward a message built from another zone's payload, only react to *whether* a topic
fired this tick.

## Rules for contributors

1. Import only `contracts/` and `core/`.
2. Do not add a new `OUTPUTS` topic other than `mission/state` to this module — that
   would make STRATEGY a data relay, which the spec forbids.
3. New phase-completion signals should be added to `INPUTS` and checked via the `seen`
   latch pattern in `step()`, not by reaching into another module's internal state.
4. On hardware (P2), this FSM splits into one local FSM per robot plus one in the ONA;
   any new transition added here is a contract both sides must implement identically.
