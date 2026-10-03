# beacon_esp — Living Map beacon (ESP32-C3)

Stores one `BeaconPayload` (Events Data + Map Data + next pointer), ages itself (relative age, no clock sync),
broadcasts every 500 ms, accepts overwrites with `version >=` stored, acks every write.
Aging *state* (fresh/aging/stale/suspect) is computed by readers (Executor, ONA) with `lm_aging` — the beacon only exposes age honestly.
Open decision: relay over the beacon chain toward B0 (diagram's "BEACONS COMS" to ONA) — TODO in `main.cpp`.
