# Wire protocol (all physical links)

Same framing on UART, USB-serial and inside ESP-NOW packets:

```
| 0xA5 | 0x5A | msg_id u8 | len u8 | payload[len] | crc16 lo | crc16 hi |
```
- payload = packed little-endian struct from `generated/c/lm_msgs.h` (`lm_msgs.py` on the Python side)
- crc16 = CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF) over `msg_id | len | payload`
- implementations: `libs/lm_embedded/lm_link.c` (C) and `libs/lm_core/link.py` (Python), cross-checked by `tests/`

| Link | Ends | Medium | Messages |
|---|---|---|---|
| `uart_writer` | Writer STM ↔ Writer Pi | UART 921600 | SensorDet, WheelOdom, ImuRaw ↑ · MotorCmd, CalibCmd, DropCmd ↓ |
| `uart_writer` | Writer Pi ↔ Writer radio ESP | USB-serial | BeaconPayload ↓ · BeaconObs, BeaconAck ↑ |
| `espnow` | radio ESPs ↔ beacons | ESP-NOW broadcast | BeaconPayload, BeaconAck, BriefHeader/Step, ActionReport |
| `uart_exec` | Executor STM ↔ Executor radio ESP | UART | BeaconObs, Brief* ↑ · BeaconPayload, ActionReport ↓ |
| `usb_ona` | ONA radio ESP ↔ ONA PC | USB-serial | BeaconObs ↑ · Brief* ↓ |
| ONA ↔ CP | ONA PC ↔ CP PC | HTTP/JSON over cellular (sat backup) | situation ↑ · mission ↓ (not framed; JSON) |

Rules: a message ID never changes meaning; add new IDs, don't repurpose. Bump `schema.yaml`, run `make gen`, commit generated files.
