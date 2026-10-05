# libs/ — shared code, linked into targets

- `lm_core/` (Python, no rclpy, no I/O): `link.py` framing, `aging.py`, `frame.py` (beacon chain → W → WGS84). Imported by `targets/writer_pi` ROS nodes, `targets/ona_pc`, `targets/cp_pc`, `sim/`.
- `lm_embedded/` (C99, no HAL): compiled into every firmware target.
  - `lm_link.c` framing, `lm_aging.c`
  - `lm_link_if.h`: the only way firmware sends/receives (`lm_send` / `lm_poll`); transports:
    `lm_lora_sx127x.c` (Ra-02 over board SPI glue, `lm_lora_as_link`) and `lm_link_uart.c` (byte stream)
  - `lm_slots.c`: LoRa TDMA (5 s period, 25 × 200 ms slots, beacon i → slot i % 24, slot 24 = readers), phase sync
  - `lm_txq.c`: readers' outgoing queue (Executor, ONA gateway), flushed only inside the reader window
  - `lm_beacon_core.c`: whole beacon behaviour (write/ack, broadcast in slot, BeaconPoll); board main is just glue

Python and C versions of the same logic are cross-checked in `tests/`. Change one → change both.
