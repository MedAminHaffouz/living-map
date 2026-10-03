# writer_stm — Writer low-level MCU (STM32)

Runs: sensor nodes (gas, temp/fire, smoke), encoders → odometry, motor PID, recalibration, beacon dropper servo.
Talks to the Pi over UART (`uart_writer` link, see `contracts/PROTOCOL.md`).

| File | Box in diagram | In | Out |
|---|---|---|---|
| `sensor_node.c` | Gaz / Temp sensor node | ADC/I²C via `board_read_*` | `SensorDet` → Pi |
| `odometry.c` | (encoders) | encoder counters | `WheelOdom` → Pi (50 Hz) |
| `motors.c` | (drive) | `MotorCmd` ← Pi (`/cmd_vel`) | PWM |
| `calib.c` | recalibration | `CalibCmd` ← Pi | baselines / resets |
| `dropper.c` | Beacon Dropper actuation | `DropCmd` ← Pi | servo |
| `app.c` | superloop + UART dispatch | | Heartbeat 1 Hz |

**Use in CubeIDE:** create the project for your board, add `app/`, `libs/lm_embedded/` and `contracts/generated/c/`
as source/include paths, implement `board_*()` from `app.h` in `board.c`, call `app_init()` / `app_tick()` in `main()`,
and `app_uart_rx_byte()` from the UART RX callback.

Add a sensor = add one `sensor_cfg_t` line in `app.c`. No new code.
