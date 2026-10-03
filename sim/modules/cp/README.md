# modules/cp/

The Command Post (far from the site): approves the mission plan the ONA carries to it,
then hands a `Brief` back to the ONA for the Executor.

| file | class | inputs | outputs | status |
|---|---|---|---|---|
| command_post.py | CommandPost | `cp/situation` -> GeoEvent | `cp/plan` -> Brief | P1 done (auto-approve all events); route/beacon_table always empty — TODO |

## Boundary

- Inbound: `cp/situation` from ONA only. Per `core/zones.py`, CP's only allowed
  publisher is ONA — there is no direct robot <-> CP link.
- Outbound: `cp/plan` to ONA only, which relays it onward as `executor/brief`.

## Rules for contributors

1. Import only `contracts/` and `core/`.
2. Do not add a direct CP -> EXECUTOR or CP -> WRITER wiring — `core/zones.py`'s
   `ALLOWED` table will reject it, and the spec requires the ONA as gateway.
3. `route` (path planning, e.g. Dijkstra over beacon topology) and `beacon_table` are
   unimplemented stubs — fill these in without changing the `cp/situation -> cp/plan`
   contract.
