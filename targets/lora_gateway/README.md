# lora_gateway — ONA LoRa ↔ USB bridge (ESP32 + Ra-02)

Plugged into the ONA PC (`usb` link, 921600 baud). `lm_link` frames on both sides.

| File | Role |
|---|---|
| `src/gateway.c` | logic, board-agnostic, host-tested (`tests/test_lora_firmware.py`) |
| `src/main.cpp` | glue: Ra-02 on VSPI, USB serial, `millis()` |

- **Air → PC:** a `BeaconPayload` becomes a `BeaconObs` with the packet RSSI, and syncs the gateway's TDMA phase.
  Everything else (acks, action reports, …) passes through unchanged.
- **PC → air:** queued (16 entries; when full, new messages are dropped and counted) and transmitted only in the
  reader window (slot 24), and only if the packet ends before the window does.
  Until the gateway has heard a beacon its phase is free-running, so the window is not yet aligned with the beacons.

| Ra-02 | ESP32 DevKit |
|---|---|
| NSS | GPIO 5 |
| RST | GPIO 14 |
| DIO0 | GPIO 26 |
| SCK / MISO / MOSI | GPIO 18 / 19 / 23 |
| 3.3 V / GND | 3V3 / GND |

```bash
pio run -e gateway -t upload
```
