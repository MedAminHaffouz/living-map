/* Host stubs for board_* so firmware app code compiles + links on a PC (CI check only). */
#include <stdint.h>
#include <stddef.h>
static uint32_t t;
uint32_t board_millis(void) { return t += 10; }
void board_uart_write(const uint8_t *d, size_t n) { (void)d; (void)n; }
int32_t board_enc_left(void) { return (int32_t)t; }
int32_t board_enc_right(void) { return (int32_t)t; }
float board_read_gas_ppm(void) { return 50.f; }
float board_read_smoke_ratio(void) { return 1.0f; }
float board_read_temp_c(void) { return 25.f; }
void board_pump(int on) { (void)on; }
void board_kit_servo_release(void) {}
int board_kit_bay_empty(void) { return 1; }
void board_drive(float v, float w) { (void)v; (void)w; }
