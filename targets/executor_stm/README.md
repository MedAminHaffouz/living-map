# executor_stm — Executor robot (STM32 + radio ESP32 over UART)

No ROS. Same superloop pattern as `writer_stm`. The STM can't do ESP-NOW, so a `radio_esp` board is the
Executor's antenna: it forwards beacon broadcasts (`BeaconObs` + RSSI) and the brief to the STM.

| File | Box in diagram |
|---|---|
| `brief_intake.c` | Brief Intake (BriefHeader + BriefStep from ONA Mission Debrief) |
| `beacon_handler.c` | Beacon Handler (table, RSSI, trust state via `lm_aging`) |
| `actions_taker.c` | Actions Taker (NAVIGATE → VERIFY if stale → ACT → REPORT, per brief step) |
| `fire_action_node.c` | Fire Action Node → extinguisher pump |
| `first_aid_node.c` | First Aid Node → kit servo |
| `beacon_update.c` | Beacon update "with confirm" (version+1) + ActionReport |
| `nav.c` | beacon-to-beacon navigation (RSSI homing) |

Add an actuator = one new `actuator_node_t` file + one line in `node_for()`.
Every actuator owns its hard timeout: if the loop stalls, nothing stays on.
