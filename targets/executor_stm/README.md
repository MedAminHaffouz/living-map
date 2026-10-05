# executor_stm — Executor robot (STM32F103C8T6 + Ra-02 LoRa)

No ROS. Same superloop pattern as `writer_stm`. The STM drives its own Ra-02 (SX1278, 433 MHz) on SPI through
`libs/lm_embedded` (`lm_lora_sx127x` → `lm_link_if_t`); there is no radio co-processor.

**Two robots, one firmware**, chosen at build time:

| Build flag | Robot | Actuator node | Board glue it needs |
|---|---|---|---|
| `-DEXECUTOR_TYPE=EX_FIRE` | EX-F (fire) | `fire_action_node.c` → extinguisher pump | `board_pump`, `board_read_temp_c` |
| `-DEXECUTOR_TYPE=EX_MED` | EX-M (first aid) | `first_aid_node.c` → kit servo | `board_kit_servo_release`, `board_kit_bay_empty` |

Anything else is a compile `#error`. Only the matching actuator node is built (`ex_actuator()`).
A brief step that needs the *other* capability (EX-F given `FIRST_AID`, EX-M given `EXTINGUISH`) is reported
`ABORTED` right away: not driven to, not attempted, its beacon is left untouched.

| File | Box in diagram |
|---|---|
| `app.c` | radio: LoRa init (433 MHz, SF7, BW125, CR 4/5, 14 dBm, sync 0x4C), receive dispatch, TX queue |
| `brief_intake.c` | Brief Intake (BriefHeader + BriefStep from ONA Mission Debrief, over LoRa via `lora_gateway`) |
| `beacon_handler.c` | Beacon Handler (table, RSSI, trust state via `lm_aging`) |
| `actions_taker.c` | Actions Taker (NAVIGATE → VERIFY if stale → ACT → REPORT, per brief step) |
| `fire_action_node.c` | Fire Action Node → extinguisher pump (EX-F) |
| `first_aid_node.c` | First Aid Node → kit servo (EX-M) |
| `beacon_update.c` | Beacon update "with confirm" (version+1) + ActionReport |
| `nav.c` | beacon-to-beacon navigation (RSSI homing) |

**Radio:** a heard `BeaconPayload` becomes a `BeaconObs` with the packet RSSI, syncs the TDMA phase
(`lm_slot_sync`) and goes to the beacon handler; `BriefHeader` / `BriefStep` go to brief intake.
Everything the Executor sends (beacon updates, ActionReports) goes through `ex_tx()`: an 8-entry queue sent only
in the reader window (slot 24), so it never talks over a beacon.

**Use in CubeIDE:** add `app/`, `libs/lm_embedded/` (`lm_link.c`, `lm_aging.c`, `lm_lora_sx127x.c`, `lm_slots.c`,
`lm_txq.c`) and `contracts/generated/c/` as source/include paths, define `EXECUTOR_TYPE`, implement the `board_*()`
glue from `executor.h` in `board.c` (incl. `board_lora()` for the Ra-02 SPI pins), call `app_init()` / `app_tick()` in `main()`.

Add an actuator = one new `actuator_node_t` file + its `EXECUTOR_TYPE` in `ex_actuator()`.
Every actuator owns its hard timeout: if the loop stalls, nothing stays on.
