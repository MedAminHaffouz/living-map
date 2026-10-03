# modules/ona/

The ONA (Outside Network Area) gateway: Receive -> Translate -> Carry (`Translate`) and
Brief (`BriefOut`). This is the **only** path between inside-the-site traffic (WRITER/
BEACON) and outside traffic (CP/EXECUTOR-at-dock) — enforced by `core/zones.py`.

| file | class | inputs | outputs | status |
|---|---|---|---|---|
| ona.py | Translate | `ona/upload` -> MissionLog | `cp/situation` -> GeoEvent | P1 done (fixed-heading rotation + flat-earth projection) |
| ona.py | BriefOut | `cp/plan` -> Brief | `executor/brief` -> Brief | P1 done (pure pass-through) |

## Boundary

- Inbound: `ona/upload` from WRITER (the only WRITER egress). `cp/plan` from CP.
- Outbound: `cp/situation` to CP. `executor/brief` to EXECUTOR (the only ONA -> EXECUTOR
  crossing; EXECUTOR gets its brief here, at the dock, before re-entering).
- No direct WRITER <-> EXECUTOR link exists, and no direct robot <-> CP link exists —
  every hop goes through one of these two classes.

## Rules for contributors

1. Import only `contracts/` and `core/`.
2. `Translate.step()` asserts `ev.header.frame == Frame.W` on every Event — do not
   relax this; frame correctness here is what makes `GeoEvent.lat/lon` trustworthy.
3. `heading_deg`/`entrance_lat`/`entrance_lon` are fixed params in P1. P2 should derive
   heading from the Writer's actual exit pose rather than a static config value — TODO.
