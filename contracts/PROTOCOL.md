# Protocol (all physical links)

Every message that crosses a physical link is defined once in [`schema.yaml`](schema.yaml); `make gen` produces the
packed C structs (`generated/c/lm_msgs.h`), the Python dataclasses (`generated/py/lm_msgs.py`), the ROS 2 `.msg`
files and the micro-ROS converters (`generated/c/lm_uros_conv.h`).

## Links

| Link | Ends | Medium | Messages |
|---|---|---|---|
| `uros` | Writer Pi ↔ Writer STM | micro-ROS (XRCE-DDS) over UART 921600; rosidl structs, not framed | SensorDet, WheelOdom, ImuRaw, BeaconObs, BeaconAck ↑ · Twist `/cmd_vel`, DropCmd, BeaconPayload (write) ↓ · service `/stm/calibrate` |
| `lora` | Writer STM, Executors, beacons, `lora_gateway` | LoRa Ra-02 (SX1278) 433 MHz, SF7, BW 125 kHz, CR 4/5, sync word 0x4C, explicit header + PHY CRC | BeaconPayload, BeaconAck, BeaconPoll, BriefHeader/Step, ActionReport |
| `usb` | `lora_gateway` (ESP32) ↔ ONA PC | USB-serial 921600 | BeaconObs, BeaconAck, ActionReport ↑ · BriefHeader/Step ↓ |
| CP | ONA PC ↔ Command Post | HTTP/JSON over cellular; Iridium SBD backup (300-byte binary situation, `ona/cp_link.py`) | situation ↑ · mission ↓ (not framed) |

There is deliberately no Writer ↔ Executor link and no robot ↔ CP link.

## Framing (`lora`, `usb`)

```
| 0xA5 | 0x5A | msg_id u8 | len u8 | payload[len] | crc16 lo | crc16 hi |
```
- payload = packed little-endian struct from `lm_msgs.h` / `lm_msgs.py`, at most 64 bytes
- crc16 = CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF) over `msg_id | len | payload`
- one LoRa packet carries one frame (a frame never spans packets); on USB the decoder resyncs on `A5 5A`
- implementations: `libs/lm_embedded/lm_link.c` (C) and `libs/lm_core/link.py` (Python), cross-checked by `tests/`

## LoRa TDMA (`libs/lm_embedded/lm_slots.c`)

LoRa is half-duplex and a BeaconPayload frame takes 62 ms on air (SF7/BW125), so nodes take turns:

- **Period** 5000 ms, **25 slots** of 200 ms. Beacon `i` transmits in slot `i % 24`.
  **Slot 24 is the reader window**: the Writer STM, the Executors and the `lora_gateway` send only there, and only
  if the packet ends before the window does (`lm_txq`, Writer write queue).
- **No shared clock.** Every node keeps a phase offset. Every `BeaconPayload` carries the sender's `phase_ms` (its
  phase when it started sending); a receiver calls `lm_slot_sync(phase_ms, airtime, now)`: the first sample jumps,
  later ones correct by `err / 4`, with `err` wrapped to ±2500 ms.
- **Beacon broadcast:** once per period in its slot, `age_s = max(1, seconds since written)` (the beacon counts its
  own age), plus `phase_ms`. A `BeaconPoll` for its id makes it broadcast again in the next slot.
- **Write = `age_s == 0`.** A beacon accepts a `BeaconPayload` with its own id, `age_s == 0` and
  `version >=` stored, and always answers `BeaconAck (id, version, ok)`.
  The Writer STM waits for the ack up to 2 periods, **retries 3 times**, then reports `ok = 0` on `/beacon/ack`.
  The Executor overwrites with `version + 1`.
- After a reboot a beacon restores its payload from NVS with `flags` bit0 (SUSPECT) set: its age restarted.

## Rules

- A message ID never changes meaning: add new IDs, never repurpose one. Bump `schema.yaml`, run `make gen`, commit
  `generated/` (CI fails if they differ).
- Wire structs are packed; micro-ROS structs are not. **Never memcpy between wire and ROS structs**: use the generated
  `lm_uros_conv.h` (field by field). Wire → wire (BeaconPayload → BeaconObs prefix) is fine and static-asserted.
- Firmware never talks to a radio or UART directly: it goes through `lm_link_if_t`.
