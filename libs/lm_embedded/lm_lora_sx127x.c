#include "lm_lora_sx127x.h"

enum {  /* SX127x LoRa-mode registers */
    REG_FIFO = 0x00, REG_OP_MODE = 0x01, REG_FRF_MSB = 0x06, REG_FRF_MID = 0x07, REG_FRF_LSB = 0x08,
    REG_PA_CONFIG = 0x09, REG_FIFO_ADDR_PTR = 0x0D, REG_FIFO_TX_BASE = 0x0E, REG_FIFO_RX_BASE = 0x0F,
    REG_FIFO_RX_CURRENT = 0x10, REG_IRQ_FLAGS = 0x12, REG_RX_NB_BYTES = 0x13, REG_PKT_RSSI = 0x1A,
    REG_MODEM_CONFIG1 = 0x1D, REG_MODEM_CONFIG2 = 0x1E, REG_PREAMBLE_MSB = 0x20, REG_PREAMBLE_LSB = 0x21,
    REG_PAYLOAD_LENGTH = 0x22, REG_MODEM_CONFIG3 = 0x26, REG_DETECT_OPTIMIZE = 0x31, REG_DETECT_THRESHOLD = 0x37,
    REG_SYNC_WORD = 0x39, REG_DIO_MAPPING1 = 0x40, REG_VERSION = 0x42,
};
enum { MODE_LORA = 0x80, MODE_LF = 0x08, MODE_SLEEP = 0x00, MODE_STDBY = 0x01, MODE_TX = 0x03, MODE_RX_CONT = 0x05 };
enum { IRQ_RX_DONE = 0x40, IRQ_CRC_ERR = 0x20, IRQ_TX_DONE = 0x08 };
enum { DIO0_RX_DONE = 0x00, DIO0_TX_DONE = 0x40 };
#define LF_LIMIT_HZ 525000000u   /* below: LF port + RSSI offset -164 instead of -157 */

static const uint32_t BW_HZ[] = { 7800, 10400, 15600, 20800, 31250, 41700, 62500, 125000, 250000, 500000 };

static uint8_t rd(lm_lora_t *r, uint8_t a) {
    r->b->nss(0); r->b->spi_xfer(a & 0x7F); uint8_t v = r->b->spi_xfer(0); r->b->nss(1); return v;
}
static void wr(lm_lora_t *r, uint8_t a, uint8_t v) {
    r->b->nss(0); r->b->spi_xfer(a | 0x80); r->b->spi_xfer(v); r->b->nss(1);
}
static void wait_ms(const lm_lora_board_t *b, uint32_t ms) { uint32_t t = b->millis(); while (b->millis() - t < ms) {} }
static int bw_code(uint32_t bw) {
    for (unsigned i = 0; i < sizeof BW_HZ / sizeof *BW_HZ; i++) if (BW_HZ[i] == bw) return (int)i;
    return -1;
}
static int ldro(const lm_lora_cfg_t *c) { return c->sf >= 11 && c->bw_hz <= 125000; }
static uint8_t mode(const lm_lora_t *r, uint8_t m) { return (uint8_t)(MODE_LORA | (r->cfg.freq_hz < LF_LIMIT_HZ ? MODE_LF : 0) | m); }
static void start_rx(lm_lora_t *r) {
    wr(r, REG_OP_MODE, mode(r, MODE_STDBY));
    wr(r, REG_DIO_MAPPING1, DIO0_RX_DONE);
    wr(r, REG_FIFO_ADDR_PTR, 0x00);
    wr(r, REG_OP_MODE, mode(r, MODE_RX_CONT));
}

uint32_t lm_lora_airtime_ms(const lm_lora_cfg_t *c, uint8_t n) {
    /* Semtech AN1200.13: T = (n_preamble + 4.25 + 8 + max(ceil((8PL - 4SF + 28 + 16CRC - 20IH) / (4(SF - 2DE))) * CR, 0)) * 2^SF / BW
       counted in quarter symbols to stay integer; CRC = 1, IH = 0, CR = 4/cr -> (cr) symbols per block */
    int32_t num = 8 * n - 4 * c->sf + 28 + 16, den = 4 * (c->sf - 2 * ldro(c));
    int32_t blocks = num > 0 ? (num + den - 1) / den : 0;
    uint64_t quarter_syms = 4u * LM_LORA_PREAMBLE + 17 + 4u * (8 + (uint32_t)blocks * c->cr);
    uint64_t den_ms = 4ull * c->bw_hz;
    return (uint32_t)((quarter_syms * (1ull << c->sf) * 1000ull + den_ms - 1) / den_ms);
}

int lm_lora_init(lm_lora_t *r, const lm_lora_board_t *b, const lm_lora_cfg_t *cfg) {
    int bw = bw_code(cfg->bw_hz);
    if (bw < 0 || cfg->sf < 7 || cfg->sf > 12 || cfg->cr < 5 || cfg->cr > 8) return LM_LORA_ERR_CFG;
    r->b = b; r->cfg = *cfg; r->tx_timeouts = r->crc_errors = 0; lm_decoder_init(&r->dec);
    if (r->cfg.tx_dbm < 2) r->cfg.tx_dbm = 2;
    if (r->cfg.tx_dbm > 17) r->cfg.tx_dbm = 17;
    b->nss(1); b->reset(0); wait_ms(b, 1); b->reset(1); wait_ms(b, 10);
    if (rd(r, REG_VERSION) != 0x12) return LM_LORA_ERR_VERSION;

    wr(r, REG_OP_MODE, mode(r, MODE_SLEEP));           /* LoRa bit only writable in sleep */
    uint64_t frf = ((uint64_t)r->cfg.freq_hz << 19) / 32000000u;
    wr(r, REG_FRF_MSB, (uint8_t)(frf >> 16)); wr(r, REG_FRF_MID, (uint8_t)(frf >> 8)); wr(r, REG_FRF_LSB, (uint8_t)frf);
    wr(r, REG_FIFO_TX_BASE, 0x00); wr(r, REG_FIFO_RX_BASE, 0x00);
    wr(r, REG_PA_CONFIG, (uint8_t)(0x80 | (r->cfg.tx_dbm - 2)));                      /* PA_BOOST, Pout = 2 + OutputPower */
    wr(r, REG_MODEM_CONFIG1, (uint8_t)(bw << 4 | (r->cfg.cr - 4) << 1));               /* bit0 = 0: explicit header */
    wr(r, REG_MODEM_CONFIG2, (uint8_t)(r->cfg.sf << 4 | 0x04));                         /* RxPayloadCrcOn */
    wr(r, REG_MODEM_CONFIG3, (uint8_t)(0x04 | (ldro(&r->cfg) ? 0x08 : 0)));             /* AGC auto, LDRO */
    wr(r, REG_DETECT_OPTIMIZE, 0x03); wr(r, REG_DETECT_THRESHOLD, 0x0A);                /* SF7..12 values */
    wr(r, REG_PREAMBLE_MSB, 0); wr(r, REG_PREAMBLE_LSB, LM_LORA_PREAMBLE);
    wr(r, REG_SYNC_WORD, LM_LORA_SYNC_WORD);
    wr(r, REG_IRQ_FLAGS, 0xFF);
    start_rx(r);
    return LM_LORA_OK;
}

int lm_lora_send(lm_lora_t *r, uint8_t id, const void *payload, uint8_t len) {
    if (len > LM_MAX_PAYLOAD) return LM_LORA_ERR_LEN;
    uint8_t f[LM_MAX_PAYLOAD + 6]; uint8_t n = (uint8_t)lm_encode(id, payload, len, f);
    wr(r, REG_OP_MODE, mode(r, MODE_STDBY));
    wr(r, REG_FIFO_ADDR_PTR, 0x00);
    r->b->nss(0); r->b->spi_xfer(REG_FIFO | 0x80);
    for (uint8_t i = 0; i < n; i++) r->b->spi_xfer(f[i]);
    r->b->nss(1);
    wr(r, REG_PAYLOAD_LENGTH, n);
    wr(r, REG_DIO_MAPPING1, DIO0_TX_DONE);
    wr(r, REG_IRQ_FLAGS, IRQ_TX_DONE);
    wr(r, REG_OP_MODE, mode(r, MODE_TX));
    uint32_t t0 = r->b->millis(), timeout = 2 * lm_lora_airtime_ms(&r->cfg, n) + 50;
    int rc = LM_LORA_OK;
    while (!(rd(r, REG_IRQ_FLAGS) & IRQ_TX_DONE))
        if (r->b->millis() - t0 > timeout) { r->tx_timeouts++; rc = LM_LORA_ERR_TIMEOUT; break; }
    wr(r, REG_IRQ_FLAGS, IRQ_TX_DONE);
    start_rx(r);
    return rc;
}

typedef struct { lm_link_rx_cb cb; void *user; int8_t rssi; } rx_ctx_t;
static void on_frame(uint8_t id, const uint8_t *p, uint8_t len, void *ctx) {
    const rx_ctx_t *x = ctx; x->cb(id, p, len, x->rssi, x->user);
}

void lm_lora_poll(lm_lora_t *r, lm_link_rx_cb cb, void *user) {
    if (r->b->dio0 && !r->b->dio0()) return;           /* fast path: no SPI traffic while idle */
    uint8_t irq = rd(r, REG_IRQ_FLAGS);
    if (!(irq & IRQ_RX_DONE)) return;
    wr(r, REG_IRQ_FLAGS, irq);
    if (irq & IRQ_CRC_ERR) { r->crc_errors++; return; }
    uint8_t n = rd(r, REG_RX_NB_BYTES), buf[255];
    wr(r, REG_FIFO_ADDR_PTR, rd(r, REG_FIFO_RX_CURRENT));
    r->b->nss(0); r->b->spi_xfer(REG_FIFO & 0x7F);
    for (uint8_t i = 0; i < n; i++) buf[i] = r->b->spi_xfer(0);
    r->b->nss(1);
    int rssi = -157 + rd(r, REG_PKT_RSSI) - (r->cfg.freq_hz < LF_LIMIT_HZ ? 7 : 0);
    rx_ctx_t x = { cb, user, (int8_t)(rssi < -128 ? -128 : rssi) };
    lm_decoder_init(&r->dec);                          /* frames never span packets */
    for (uint8_t i = 0; i < n; i++) lm_decoder_feed(&r->dec, buf[i], on_frame, &x);
}

static int link_send(void *ctx, uint8_t id, const void *p, uint8_t len) { return lm_lora_send(ctx, id, p, len); }
static void link_poll(void *ctx, lm_link_rx_cb cb, void *user) { lm_lora_poll(ctx, cb, user); }
lm_link_if_t lm_lora_as_link(lm_lora_t *r) { return (lm_link_if_t){ r, link_send, link_poll }; }
