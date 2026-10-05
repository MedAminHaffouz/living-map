# Wire protocol (all physical links)

Same framing on USB-serial and inside LoRa packets (`uros` carries the rosidl structs, see `generated/c/lm_uros_conv.h`):

```
| 0xA5 | 0x5A | msg_id u8 | len u8 | payload[len] | crc16 lo | crc16 hi |
```
- payload = packed little-endian struct from `generated/c/lm_msgs.h` (`lm_msgs.py` on the Python side)
- crc16 = CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF) over `msg_id | len | payload`
- implementations: `libs/lm_embedded/lm_link.c` (C) and `libs/lm_core/link.py` (Python), cross-checked by `tests/`

| Link | Ends | Medium | Messages |
|---|---|---|---|
| `uros` | Writer Pi ↔ Writer STM | micro-ROS over UART 921600 (rosidl structs, not framed) | SensorDet, WheelOdom, ImuRaw, BeaconObs, BeaconAck ↑ · MotorCmd, DropCmd, BeaconPayload ↓ · calib = ROS service |
| `lora` | Writer STM, Executors, beacons, lora_gateway | LoRa Ra-02 (SX1278, 433 MHz) | BeaconPayload, BeaconAck, BeaconPoll, BriefHeader/Step, ActionReport, Heartbeat |
| `usb` | lora_gateway ESP32 ↔ ONA PC | USB-serial | BeaconObs, ActionReport ↑ · Brief* ↓ |
| ONA ↔ CP | ONA PC ↔ CP PC | HTTP/JSON over cellular (sat backup) | situation ↑ · mission ↓ (not framed; JSON) |

Rules: a message ID never changes meaning; add new IDs, don't repurpose. Bump `schema.yaml`, run `make gen`, commit generated files.
