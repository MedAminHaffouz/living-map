/* Writer STM application: cooperative superloop. Call app_init() once and app_tick() from main()'s while(1).
   Talks to the Pi only through uplink.h (micro-ROS in uros_app.c) and to the beacons only through LoRa.
   Board glue (HAL reads, Ra-02 SPI pins) is provided by the board_* functions you implement in board.c (CubeIDE project). */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include "lm_lora_sx127x.h"
void app_init(void);
void app_tick(void);
/* --- implement these in board.c --- */
uint32_t board_millis(void);
const lm_lora_board_t *board_lora(void);          /* Ra-02: spi_xfer, nss, reset, dio0, millis */
int32_t  board_enc_left(void), board_enc_right(void);
float    board_read_gas_ppm(void), board_read_smoke_ratio(void), board_read_temp_c(void);
int      board_imu_read(float acc[3], float gyro[3]);   /* 1 = new sample (m/s², rad/s) */
