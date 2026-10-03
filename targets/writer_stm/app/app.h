/* Writer STM application: cooperative superloop. Call app_init() once and app_tick() from main()'s while(1).
   Board glue (HAL reads, UART TX) is provided by the board_* functions you implement in board.c (CubeIDE project). */
#pragma once
#include <stdint.h>
#include <stddef.h>
void app_init(void);
void app_tick(void);
void app_uart_rx_byte(uint8_t b);       /* call from UART RX ISR / DMA */
void app_sensor_baseline(uint8_t target);
void app_odom_reset(void);
/* --- implement these in board.c --- */
uint32_t board_millis(void);
void     board_uart_write(const uint8_t *d, size_t n);
int32_t  board_enc_left(void), board_enc_right(void);
float    board_read_gas_ppm(void), board_read_smoke_ratio(void), board_read_temp_c(void);
