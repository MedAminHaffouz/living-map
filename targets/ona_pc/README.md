# ona_pc — Outside Network Area (laptop + lora_gateway on USB)

| File | Box |
|---|---|
| `ona/beacons_reader.py` | Beacons reader (from `targets/lora_gateway` over USB) |
| `ona/area_knowledge.py` + `config/area.yaml` | Area Knowledge (entrance GPS, heading, notes) |
| `ona/situation_debrief.py` | Situation debrief: beacon chain → frame W → WGS84 + aging state → CP |
| `ona/mission_debrief.py` | Mission debrief: CP mission → BriefHeader/BriefStep → gateway → Executor |
| `ona/cp_link.py` | CP link: `CpLink` protocol; `HttpCellular` (primary), `SatelliteSBD` (backup, 300-byte `compact()` situation, modem driver TODO), `Failover` (sticks to the working link, re-probes cellular every 30 s) |

`config/area.yaml`: `entrance`, `heading_deg`, `cp_url`, `sat_port`, `gateway_port`.

Run: `pip install pyserial pyyaml && python -m ona.main --area config/area.yaml`
Why a PC and not an ESP/STM: translation, JSON/HTTP to the CP and logging are trivial here and painful on an MCU; the ESP32 + Ra-02 gateway is the radio.
