/* Host stubs for board_* so firmware app code compiles + links on a PC (CI check only). */
#include <stdint.h>
#include <stddef.h>
#include "lm_lora_sx127x.h"
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

/* Ra-02 stand-in: register file that answers version 0x12, IRQ flags write-1-to-clear, TX completes at once.
   Enough for lm_lora_init / send / poll to run; tests/c/test_lora.c has the detailed fake. */
static uint8_t reg[128], fifo[256]; static int first, addr, wr;
static uint8_t spi_xfer(uint8_t b) {
    if (first) { first = 0; addr = b & 0x7F; wr = b & 0x80; return 0; }
    if (addr == 0x00) { uint8_t *p = &reg[0x0D]; if (wr) { fifo[(*p)++] = b; return 0; } return fifo[(*p)++]; }
    if (!wr) return addr == 0x42 ? 0x12 : reg[addr];
    if (addr == 0x12) reg[addr] &= (uint8_t)~b;
    else { reg[addr] = b; if (addr == 0x01 && (b & 0x07) == 0x03) reg[0x12] |= 0x08; }
    return 0;
}
static void nss(int l) { if (!l) first = 1; }
static void rst(int l) { (void)l; }
static int dio0(void) { return (reg[0x12] & 0x48) != 0; }
static uint32_t ms(void) { return ++t; }
static const lm_lora_board_t LORA = { spi_xfer, nss, rst, dio0, ms };
const lm_lora_board_t *board_lora(void) { return &LORA; }
