# modules/beacon/

RF beacons deployed inside the site. Simulated in P1 as a single process holding every
beacon's latest state; on hardware (P2) each beacon is its own ESP32-C3.

| file | class | inputs | outputs | status |
|---|---|---|---|---|
| beacon_field.py | BeaconField | `beacon/write` (from WRITER), `beacon/update` (from EXECUTOR) | `beacon/broadcast` -> BeaconObs | P1 done (sim); P2: real firmware per beacon |

## Boundary

- Inbound: `beacon/write` (WRITER drops/writes a beacon), `beacon/update` (EXECUTOR
  overwrites a stale/contradicted beacon, version+1).
- Outbound: `beacon/broadcast` — heard by anyone inside in radio range. Per
  `core/zones.py`, BEACON may be heard by WRITER, EXECUTOR, and ONA (gateway hears B0
  only in the real system; P1 sim broadcasts every beacon to every listener uniformly).

This is the one zone both WRITER and EXECUTOR touch — but never directly: they write/
read beacon state, never message each other.

## Rules for contributors

1. Import only `contracts/` and `core/`.
2. Never drop a beacon version on write — `BeaconField` must only accept a write/update
   if `incoming.version >= stored.version` (see `beacon_field.py:step`).
3. `rssi` is a fixed constant in P1 (`-50.0`); do not special-case it per beacon id —
   P2 replaces the whole module with real radio hardware, so sim-side RSSI modeling is
   out of scope here.
