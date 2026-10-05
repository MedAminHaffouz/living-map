# Living Map — rules for agents
Hardware: Writer = Raspberry Pi (ROS 2) + STM32 NUCLEO-L4R5ZI; Pi<->STM = micro-ROS over UART (921600).
Writer STM drives a LoRa Ra-02 (SX1278, 433 MHz) on SPI and writes the beacons.
Beacons = ESP32-C3 + Ra-02. Executors = STM32F103C8T6 + Ra-02, no ROS, two types: EX-F (fire) and EX-M (first aid),
selected with -DEXECUTOR_TYPE=EX_FIRE|EX_MED. ONA = PC + lora_gateway (ESP32 + Ra-02 on USB).
ONA<->CP = cellular HTTP, satellite (Iridium SBD) as backup.
All radio links are LoRa: Writer->beacons, beacon<->beacon, Executor<->beacons, ONA<->beacons, ONA<->Executor.
Rules:
- Every cross-device message lives in contracts/schema.yaml. Never hand-write a struct; run `make gen`, commit contracts/generated.
- Wire structs are packed; micro-ROS structs are not. Never memcpy between them; use the generated converters.
- Firmware never talks to a radio/UART directly: it goes through lm_link_if_t (libs/lm_embedded).
- Logic lives in libs/ and app files; board glue (pins, HAL) lives only in board.c / main.cpp.
- `make test` must pass before every push. One feature per branch, then a PR.
