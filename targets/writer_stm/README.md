# writer_stm — Writer low-level MCU (STM32 NUCLEO-L4R5ZI)

Runs: sensor nodes (gas, temp/fire, smoke), encoders → odometry, motor PID, IMU, recalibration, beacon dropper servo,
and the Writer's LoRa radio (Ra-02 on SPI) that writes the beacons.
Talks to the Pi with **micro-ROS** over UART 921600 (`uros` link, see `contracts/PROTOCOL.md`).

```
 sensors / odom / IMU / LoRa ──► app.c ──uplink_*()──► uros_app.c ──micro-ROS──► Pi
                                  ▲                         │
                                  └──── app_on_*() ◄────────┘
```
`uplink.h` is the seam: `app.c` never sees a ROS type, `uros_app.c` is the **only** file that includes ROS headers
(checked by `tests/test_writer_stm.py`). Wire structs ↔ ROS structs only through the generated `lm_uros_conv.h`.

| File | Role | In | Out |
|---|---|---|---|
| `sensor_node.c` | Gas / Temp / Smoke sensor node | ADC/I²C via `board_read_*` | `uplink_sensor_det` |
| `odometry.c` | encoders → odometry | encoder counters | `uplink_wheel_odom` (50 Hz) |
| `motors.c` | drive | `app_on_cmd_vel` (`/cmd_vel`) | PWM; stops after 300 ms without a command or on link loss |
| `dropper.c` | Beacon Dropper actuation | `app_on_drop` (`/stm/drop`) | servo |
| `app.c` | superloop, IMU, calibration, LoRa beacon writes | `app_on_*` | `uplink_*`, LoRa |
| `uros_app.c` | micro-ROS node `writer_stm` | Pi | Pi |

**ROS interface** (node `writer_stm`)

| | Name | Type | QoS |
|---|---|---|---|
| pub | `/stm/sensor_det` | `lm_interfaces/SensorDet` | best effort |
| pub | `/stm/wheel_odom` | `lm_interfaces/WheelOdom` | best effort |
| pub | `/stm/imu` | `lm_interfaces/ImuRaw` | best effort |
| pub | `/beacon/obs` | `lm_interfaces/BeaconObs` (beacon heard + RSSI) | reliable |
| pub | `/beacon/ack` | `lm_interfaces/BeaconAck`: one per `/beacon/write` | reliable |
| sub | `/cmd_vel` | `geometry_msgs/Twist` | |
| sub | `/stm/drop` | `lm_interfaces/DropCmd` | |
| sub | `/beacon/write` | `lm_interfaces/BeaconPayload` | |
| srv | `/stm/calibrate` | `lm_interfaces/srv/Calibrate` | `ENCODER_RESET` resets odometry; `ZERO_BASELINE`, `IMU_BIAS` are TODO (return ok) |

**Beacon writes:** up to 8 queued; each is sent in the TDMA reader window with `age_s = 0` and our `phase_ms`, then
we wait 2 periods (10 s) for a `BeaconAck` with the same `(id, version)`. 3 retries, then `/beacon/ack` with `ok = 0`.
A beacon's own ack (ok or refused) is forwarded as is. Queue full → immediate `ok = 0`.

**Agent link:** `WAIT_AGENT` (ping every 500 ms) → `CONNECTED` (ping every 1 s, 3 attempts) → `LOST`:
motors stopped (`app_on_link_lost`), entities destroyed, back to `WAIT_AGENT`. Uplinks are dropped while not connected.

## CubeIDE project

1. **CubeMX** (board NUCLEO-L4R5ZI):
   - the UART wired to the Pi: 921600 8N1, DMA RX *circular*, DMA TX *normal*, UART global interrupt on
   - SPI for the Ra-02 + 3 GPIOs (NSS out, RST out, DIO0 in)
   - timers for encoders / PWM, ADC for the gas & smoke sensors, I²C for the IMU
   - FreeRTOS (CMSIS v2) with one task, stack ≥ 3000 words: micro-ROS' allocators and transport expect it
2. **micro-ROS static library, with `lm_interfaces` built in.** micro-ROS on the MCU only knows the message types
   compiled into its static library, so `lm_interfaces` has to be added as an *extra package*:
   ```bash
   cd <cubeide_project>
   git clone -b humble https://github.com/micro-ROS/micro_ros_stm32cubemx_utils.git   # same distro as the Pi
   make gen -C <living-map>                                                             # fresh .msg files first
   cp -r <living-map>/targets/writer_pi/ros2_ws/src/lm_interfaces \
         micro_ros_stm32cubemx_utils/microros_static_library_ide/library_generation/extra_packages/
   ```
   Copy, don't symlink: the library is built inside Docker, which only sees the project folder.
   After any change to `contracts/schema.yaml` or `Calibrate.srv`: `make gen`, copy again, and delete
   `micro_ros_stm32cubemx_utils/microros_static_library_ide/libmicroros/` to force a rebuild.
3. **Project properties** (C/C++ Build → Settings), as in the micro_ros_stm32cubemx_utils README (Docker must be running):
   - Pre-build step:
     `make -f ../micro_ros_stm32cubemx_utils/microros_static_library_ide/libmicroros.mk`
   - Include path: `../micro_ros_stm32cubemx_utils/microros_static_library_ide/libmicroros/include`
   - Library search path `../micro_ros_stm32cubemx_utils/microros_static_library_ide/libmicroros`, library `microros`
   - Add from `micro_ros_stm32cubemx_utils/extra_sources/`: `microros_time.c`, `microros_allocators.c`,
     `custom_memory_manager.c`, `microros_transports/dma_transport.c`
4. **Living Map sources:** add `app/` (all files, incl. `uros_app.c`), from `libs/lm_embedded/`: `lm_link.c`,
   `lm_lora_sx127x.c`, `lm_slots.c`; include paths `app/`, `libs/lm_embedded/`, `contracts/generated/c/`.
5. **board.c:** implement every `board_*()` from `app.h` (`board_lora()` returns the Ra-02 SPI/GPIO glue).
6. **The task** (`main.c` / `freertos.c`): set up the transport and allocator exactly as the utils' sample does
   (`rmw_uros_set_custom_transport(true, &huartX, cubemx_transport_open, cubemx_transport_close,
   cubemx_transport_write, cubemx_transport_read)` and `rcutils_set_default_allocator(...)` with the `microros_*`
   allocators), then:
   ```c
   app_init(); uros_app_init();
   for (;;) { app_tick(); uros_app_tick(); osDelay(1); }
   ```
7. **Pi:** `ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyAMA0 -b 921600`, with the Pi's
   `lm_interfaces` built from the same `make gen`.

Add a sensor = add one `sensor_cfg_t` line in `app.c`. No new code.

Host checks (`make test`): the app (everything but `uros_app.c`) is built and run with `tests/c/uplink_stub.c`, and
`uros_app.c` is compiled with `-fsyntax-only` against declaration stubs of the rcl/rclc/rmw_microros API
(`tests/uros_stub/`). That catches API and converter mismatches, but only a CubeIDE build links the real library.
