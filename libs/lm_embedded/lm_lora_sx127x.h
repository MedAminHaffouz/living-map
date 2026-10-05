/* Minimal SX1276/77/78 (Ra-02) LoRa driver. One radio packet = one lm_link frame (or several back to back).
   Explicit header, PHY CRC on, sync word 0x4C, PA_BOOST output. Board glue only through lm_lora_board_t. */
#pragma once
#include <stdint.h>
#include "lm_link.h"
#include "lm_link_if.h"

#define LM_LORA_SYNC_WORD 0x4C
#define LM_LORA_PREAMBLE  8
/* the one radio config every Living Map node uses (Ra-02 433 MHz); a mismatch = deaf nodes */
#define LM_LORA_CFG_DEFAULT { 433000000, 7, 125000, 5, 17 }

typedef struct {
    uint8_t  (*spi_xfer)(uint8_t b);   /* full-duplex byte */
    void     (*nss)(int level);        /* chip select, active low */
    void     (*reset)(int level);      /* NRESET pin, active low */
    int      (*dio0)(void);            /* DIO0 level; NULL = poll the IRQ register instead */
    uint32_t (*millis)(void);
} lm_lora_board_t;

typedef struct {
    uint32_t freq_hz;   /* e.g. 433000000 */
    uint8_t  sf;        /* 7..12 */
    uint32_t bw_hz;     /* 7800, 10400, 15600, 20800, 31250, 41700, 62500, 125000, 250000, 500000 */
    uint8_t  cr;        /* coding rate denominator: 5..8 = 4/5..4/8 */
    int8_t   tx_dbm;    /* PA_BOOST, clamped to 2..17 */
} lm_lora_cfg_t;

typedef struct {
    const lm_lora_board_t *b;
    lm_lora_cfg_t cfg;
    lm_decoder_t dec;
    uint32_t tx_timeouts, crc_errors;
} lm_lora_t;

enum { LM_LORA_OK = 0, LM_LORA_ERR_VERSION = -1, LM_LORA_ERR_CFG = -2, LM_LORA_ERR_LEN = -3, LM_LORA_ERR_TIMEOUT = -4 };

/* resets the chip, checks version reg 0x42 == 0x12, configures it, leaves it in RX continuous */
int  lm_lora_init(lm_lora_t *r, const lm_lora_board_t *b, const lm_lora_cfg_t *cfg);
/* blocking: frames + transmits, waits TxDone (timeout 2x airtime + 50 ms), back to RX continuous */
int  lm_lora_send(lm_lora_t *r, uint8_t id, const void *payload, uint8_t len);
/* non-blocking: on RxDone reads the packet, drops CRC errors, calls cb for every valid frame with the packet RSSI */
void lm_lora_poll(lm_lora_t *r, lm_link_rx_cb cb, void *user);
/* Semtech time-on-air for an n-byte PHY payload (explicit header, CRC on, LDRO per cfg), rounded up */
uint32_t lm_lora_airtime_ms(const lm_lora_cfg_t *cfg, uint8_t n_bytes);
lm_link_if_t lm_lora_as_link(lm_lora_t *r);
