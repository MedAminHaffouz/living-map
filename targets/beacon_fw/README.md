# beacon_fw — Living Map beacon (ESP32-C3 + Ra-02)

`src/main.cpp` is board glue only: Ra-02 on SPI, `millis()`, NVS (`Preferences`). Everything a beacon *does* is
`libs/lm_embedded/lm_beacon_core.c` (host-tested in `tests/test_lm_embedded_lora.py`):

- accepts a `BeaconPayload` for its own id with `age_s == 0` (a write) and `version >=` stored; acks every write `(id, version, ok)`
- broadcasts once per 5 s TDMA period in slot `id % 24`, with `age_s = max(1, s since write)` and its `phase_ms`
- syncs its TDMA phase on every payload it hears; answers a `BeaconPoll` for its id in the next slot
- aging *state* (fresh/aging/stale/suspect) is computed by readers with `lm_aging`; the beacon only reports age

**Reboot:** the last write is persisted in NVS and restored at boot with `flags` bit0 (SUSPECT) set, because the
age restarts and can't be trusted. The next write (Writer or Executor) clears it.

| Ra-02 | ESP32-C3 DevKitM-1 |
|---|---|
| NSS | GPIO 7 |
| RST | GPIO 3 |
| DIO0 | GPIO 2 |
| SCK / MISO / MOSI | GPIO 4 / 5 / 6 |
| 3.3 V / GND | 3V3 / GND |

Radio: `LM_LORA_CFG_DEFAULT` (433 MHz, SF7, BW 125 kHz, CR 4/5, 17 dBm, sync 0x4C) — shared by every node.

```bash
pio run -e beacon -t upload --project-option="build_flags=-I../../libs/lm_embedded -I../../contracts/generated/c -DBEACON_ID=3"
```
