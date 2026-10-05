# ona_pc — Outside Network Area (laptop + lora_gateway on USB)

| File | Box |
|---|---|
| `ona/beacons_reader.py` | Beacons reader (from `targets/lora_gateway` over USB) |
| `ona/area_knowledge.py` + `config/area.yaml` | Area Knowledge (entrance GPS, heading, notes) |
| `ona/situation_debrief.py` | Situation debrief: beacon chain → frame W → WGS84 + aging state → CP |
| `ona/mission_debrief.py` | Mission debrief: CP mission → BriefHeader/BriefStep → Executor |

Run: `pip install pyserial pyyaml && python -m ona.main --area config/area.yaml`
Why a PC and not an ESP/STM: translation, JSON/HTTP to the CP and logging are trivial here and painful on an MCU; the ESP is still needed as the radio.
