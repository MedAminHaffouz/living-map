# modules/executor/

The Executor robot: enters pre-briefed (from the ONA, at the dock), navigates by
beacon, and trusts beacons by age — overwriting stale or contradicted ones.

| file | class | inputs | outputs | status |
|---|---|---|---|---|
| executor.py | Executor | `executor/brief` -> Brief, `beacon/broadcast` -> BeaconObs | `beacon/update` -> BeaconWrite | P1 done (age classification + stale overwrite); no FRESH-beacon navigation logic — TODO |

## Boundary

- Inbound: `executor/brief` from ONA (the only way the Executor is briefed — no direct
  WRITER link). `beacon/broadcast` from BEACON.
- Outbound: `beacon/update` to BEACON only. Per `core/zones.py`, EXECUTOR's only
  reachable zones are itself and BEACON — no EXECUTOR -> WRITER, EXECUTOR -> CP, or
  EXECUTOR -> ONA (post-brief) link exists.

## Rules for contributors

1. Import only `contracts/` and `core/`.
2. Never publish a `beacon/update` with a lower `version` than what's already on the
   beacon — `BeaconField` relies on versions being non-decreasing.
3. `age()`'s `fresh_s`/`stale_s` thresholds come from `wiring.yaml` params; don't
   hardcode new thresholds in `executor.py`.
4. FRESH-beacon "follow" behavior (actual navigation) is not implemented — current code
   only classifies and conditionally overwrites. Add navigation without changing the
   `beacon/update` contract.
