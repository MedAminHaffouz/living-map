/* Host tests for libs/lm_embedded LoRa stack against a fake SX127x (register file + FIFO behind the SPI protocol).
   `test_lora` runs every check; `test_lora airtime` prints a time-on-air table for the Python cross-check. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lm_link.h"
#include "lm_link_uart.h"
#include "lm_lora_sx127x.h"
#include "lm_slots.h"
#include "lm_beacon_core.h"

#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); exit(1); } } while (0)

/* ---------- fake SX127x ---------- */
static uint8_t reg[128], fifo[256], version = 0x12, tx_done_works = 1;
static int first, addr, is_write;
static uint32_t t;
static uint8_t txlog[64][80], txlen[64]; static int ntx;

static void reg_write(uint8_t a, uint8_t v) {
    if (a == 0x12) { reg[a] &= (uint8_t)~v; return; }                /* IRQ flags: write 1 to clear */
    reg[a] = v;
    if (a == 0x01 && (v & 0x07) == 0x03) {                          /* TX: capture FIFO[tx_base .. +len] */
        CHECK(ntx < 64);
        memcpy(txlog[ntx], &fifo[reg[0x0E]], reg[0x22]); txlen[ntx++] = reg[0x22];
        if (tx_done_works) reg[0x12] |= 0x08;
    }
}
static uint8_t fake_spi(uint8_t b) {
    if (first) { first = 0; addr = b & 0x7F; is_write = b & 0x80; return 0; }
    if (addr == 0x00) { uint8_t *p = &reg[0x0D]; if (is_write) { fifo[(*p)++] = b; return 0; } return fifo[(*p)++]; }
    uint8_t v = 0;
    if (is_write) reg_write((uint8_t)addr, b); else v = addr == 0x42 ? version : reg[addr];
    addr++; return v;
}
static void fake_nss(int l) { if (!l) first = 1; }
static void fake_reset(int l) { (void)l; }
static int fake_dio0(void) { return (reg[0x12] & 0x48) != 0; }
static uint32_t fake_millis(void) { return ++t; }                  /* time moves on every read: busy-waits end */
static const lm_lora_board_t BOARD = { fake_spi, fake_nss, fake_reset, fake_dio0, fake_millis };

static void inject(const uint8_t *d, uint8_t n, uint8_t pkt_rssi, int crc_err) {
    memcpy(&fifo[0x80], d, n); reg[0x13] = n; reg[0x10] = 0x80; reg[0x1A] = pkt_rssi;
    reg[0x12] |= (uint8_t)(0x40 | (crc_err ? 0x20 : 0));
}
static void inject_msg(uint8_t id, const void *p, uint8_t len) { uint8_t f[80]; inject(f, (uint8_t)lm_encode(id, p, len, f), 100, 0); }

/* decode a captured TX packet into its (single) frame */
static uint8_t got_id, got[64], got_len; static int got_n;
static void grab(uint8_t id, const uint8_t *p, uint8_t len, void *ctx) { (void)ctx; got_id = id; memcpy(got, p, len); got_len = len; got_n++; }
static int decode_tx(int i) { lm_decoder_t d; lm_decoder_init(&d); got_n = 0; for (int k = 0; k < txlen[i]; k++) lm_decoder_feed(&d, txlog[i][k], grab, 0); return got_n == 1; }

static const lm_lora_cfg_t CFG = { 433000000, 7, 125000, 5, 20 };

/* ---------- tests ---------- */
static void test_init(void) {
    lm_lora_t r; lm_lora_cfg_t c = CFG;
    version = 0x11; CHECK(lm_lora_init(&r, &BOARD, &c) == LM_LORA_ERR_VERSION); version = 0x12;
    c.sf = 6; CHECK(lm_lora_init(&r, &BOARD, &c) == LM_LORA_ERR_CFG); c = CFG;
    c.bw_hz = 100000; CHECK(lm_lora_init(&r, &BOARD, &c) == LM_LORA_ERR_CFG); c = CFG;
    CHECK(lm_lora_init(&r, &BOARD, &c) == LM_LORA_OK);
    CHECK(reg[0x39] == 0x4C);                                      /* sync word */
    CHECK(reg[0x1D] == 0x72);                                      /* BW125, CR4/5, explicit header */
    CHECK(reg[0x1E] == 0x74);                                      /* SF7, PHY CRC on */
    CHECK(reg[0x26] == 0x04);                                      /* AGC, no LDRO */
    CHECK(reg[0x09] == 0x8F);                                      /* PA_BOOST, 20 dBm clamped to 17 */
    CHECK(reg[0x06] == 0x6C && reg[0x07] == 0x40 && reg[0x08] == 0x00);   /* 433 MHz */
    CHECK(reg[0x01] == 0x8D);                                      /* LoRa | LF | RX continuous */
    c.sf = 12; CHECK(lm_lora_init(&r, &BOARD, &c) == LM_LORA_OK && reg[0x26] == 0x0C);           /* LDRO on */
    c.sf = 11; c.bw_hz = 250000; CHECK(lm_lora_init(&r, &BOARD, &c) == LM_LORA_OK && reg[0x26] == 0x04);
}

static int rx_n; static int8_t rx_rssi; static uint8_t rx_id;
static void on_rx(uint8_t id, const uint8_t *p, uint8_t len, int8_t rssi, void *u) { (void)p; (void)len; (void)u; rx_n++; rx_rssi = rssi; rx_id = id; }

static void test_rx_tx(void) {
    lm_lora_t r; CHECK(lm_lora_init(&r, &BOARD, &CFG) == LM_LORA_OK);
    lm_link_if_t l = lm_lora_as_link(&r);
    lm_heartbeat_t h = { LM_NODE_ID_BEACON, 0, 42 }; uint8_t f[80]; uint8_t n = (uint8_t)lm_encode(LM_MSG_HEARTBEAT, &h, sizeof h, f);
    rx_n = 0; lm_poll(&l, on_rx, 0); CHECK(rx_n == 0);                       /* idle */
    inject(f, n, 100, 0); lm_poll(&l, on_rx, 0);
    CHECK(rx_n == 1 && rx_id == LM_MSG_HEARTBEAT && rx_rssi == -157 + 100 - 7);
    lm_poll(&l, on_rx, 0); CHECK(rx_n == 1);                                  /* RxDone was cleared */
    inject(f, n, 100, 1); lm_poll(&l, on_rx, 0); CHECK(rx_n == 1 && r.crc_errors == 1);
    memcpy(f + n, f, n); inject(f, (uint8_t)(2 * n), 100, 0); lm_poll(&l, on_rx, 0); CHECK(rx_n == 3);   /* 2 frames, 1 packet */

    ntx = 0; CHECK(lm_send(&l, LM_MSG_HEARTBEAT, &h, sizeof h) == 0);
    CHECK(ntx == 1 && decode_tx(0) && got_id == LM_MSG_HEARTBEAT && reg[0x01] == 0x8D);
    tx_done_works = 0; uint32_t t0 = t;
    CHECK(lm_send(&l, LM_MSG_HEARTBEAT, &h, sizeof h) == LM_LORA_ERR_TIMEOUT && r.tx_timeouts == 1);
    CHECK(t - t0 >= 2 * lm_lora_airtime_ms(&CFG, n) + 50 && reg[0x01] == 0x8D);   /* waited, back in RX */
    tx_done_works = 1;
    uint8_t big[LM_MAX_PAYLOAD + 1] = { 0 }; CHECK(lm_send(&l, 0x01, big, sizeof big) == LM_LORA_ERR_LEN);
}

static void test_airtime(void) {
    uint32_t a = lm_lora_airtime_ms(&CFG, sizeof(lm_beacon_payload_t) + 6);
    printf("airtime BeaconPayload SF7/BW125 = %u ms\n", (unsigned)a);
    CHECK(a >= 40 && a <= 100 && 2 * a < LM_SLOT_MS);
}

static void test_slots(void) {
    lm_slots_t s; lm_slots_init(&s);
    CHECK(lm_slot_phase(&s, 12345) == 2345 && lm_slot_current(&s, 12345) == 11);
    CHECK(lm_slot_of_beacon(3) == 3 && lm_slot_of_beacon(27) == 3);
    lm_slot_sync(&s, 1000, 60, 20000); CHECK(lm_slot_phase(&s, 20000) == 1060);   /* first: jump */
    lm_slot_sync(&s, 1100, 60, 20000); CHECK(lm_slot_phase(&s, 20000) == 1085);   /* then err/4 (100/4) */
    lm_slots_init(&s);
    lm_slot_sync(&s, 4900, 0, 0); lm_slot_sync(&s, 100, 0, 0);                   /* +200 across the wrap, not -4800 */
    CHECK(lm_slot_phase(&s, 0) == 4950);
    lm_slot_sync(&s, 4800, 0, 0); CHECK(lm_slot_phase(&s, 0) == 4913);           /* -150/4 = -37 */
}

static int n_persist;
static void persist(const lm_beacon_payload_t *p, void *ctx) { (void)p; (void)ctx; n_persist++; }

static int count_tx(int from, uint8_t id, uint8_t bid) {
    int c = 0; for (int i = from; i < ntx; i++) if (decode_tx(i) && got_id == id && got[0] == bid) c++;
    return c;
}
static void tick_ms(lm_beacon_t *b, uint32_t ms) { uint32_t end = t + ms; while (t < end) { t += 9; lm_beacon_tick(b, fake_millis()); } }

static void test_beacon(void) {
    lm_lora_t r; CHECK(lm_lora_init(&r, &BOARD, &CFG) == LM_LORA_OK);
    lm_link_if_t l = lm_lora_as_link(&r);
    lm_beacon_cfg_t bc = { 3, &l, lm_lora_airtime_ms(&CFG, sizeof(lm_beacon_payload_t) + 6), persist, 0 };
    lm_beacon_t b; lm_beacon_init(&b, &bc, NULL, t);
    ntx = 0;
    for (int i = 0; i < 5000; i++) { t += 9; lm_beacon_tick(&b, fake_millis()); }
    CHECK(ntx == 0);                                               /* unwritten beacon stays silent */

    lm_beacon_payload_t w = { .id = 3, .what = LM_EVENT_TYPE_FIRE, .prio = 3, .conf = 200, .version = 2, .phase_ms = 1000 };
    inject_msg(LM_MSG_BEACON_PAYLOAD, &w, sizeof w); lm_beacon_tick(&b, fake_millis());
    CHECK(n_persist == 1 && ntx == 1 && decode_tx(0) && got_id == LM_MSG_BEACON_ACK);
    lm_beacon_ack_t a; memcpy(&a, got, sizeof a); CHECK(a.id == 3 && a.version == 2 && a.ok == 1);

    int from = ntx; tick_ms(&b, 50000);                            /* 10 periods */
    int nb = count_tx(from, LM_MSG_BEACON_PAYLOAD, 3);
    CHECK(nb >= 9 && nb <= 11 && ntx - from == nb);
    uint32_t last_age = 0;
    for (int i = from; i < ntx; i++) {
        lm_beacon_payload_t p; CHECK(decode_tx(i)); memcpy(&p, got, sizeof p);
        CHECK(p.phase_ms >= 3 * LM_SLOT_MS && p.phase_ms < 4 * LM_SLOT_MS);   /* slot 3 */
        CHECK(p.age_s >= 1 && p.age_s > last_age && p.version == 2 && p.what == LM_EVENT_TYPE_FIRE);
        last_age = p.age_s;
    }

    w.version = 1; from = ntx;                                     /* older write: refused but acked */
    inject_msg(LM_MSG_BEACON_PAYLOAD, &w, sizeof w); lm_beacon_tick(&b, fake_millis());
    CHECK(ntx == from + 1 && decode_tx(from) && got_id == LM_MSG_BEACON_ACK && got[2] == 0 && n_persist == 1);
    w.version = 2; w.age_s = 7; from = ntx;                        /* someone's broadcast of my id: not a write */
    inject_msg(LM_MSG_BEACON_PAYLOAD, &w, sizeof w); lm_beacon_tick(&b, fake_millis()); CHECK(ntx == from);

    while (lm_slot_current(&b.slots, t) != 10) tick_ms(&b, 10);
    lm_beacon_poll_t q = { 4, LM_NODE_ID_EXECUTOR }; from = ntx;  /* not mine */
    inject_msg(LM_MSG_BEACON_POLL, &q, sizeof q); tick_ms(&b, 400); CHECK(ntx == from);
    while (lm_slot_current(&b.slots, t) != 10) tick_ms(&b, 10);
    q.id = 3; from = ntx; inject_msg(LM_MSG_BEACON_POLL, &q, sizeof q); lm_beacon_tick(&b, fake_millis());
    CHECK(ntx == from); tick_ms(&b, 400);                           /* not right away: slot 11 answers */
    CHECK(count_tx(from, LM_MSG_BEACON_PAYLOAD, 3) == 1 && ntx == from + 1 && decode_tx(from));
    { lm_beacon_payload_t p; memcpy(&p, got, sizeof p); CHECK(p.phase_ms / LM_SLOT_MS == 11); }

    lm_beacon_payload_t saved = b.pl; saved.age_s = 30;            /* reboot with persisted payload */
    lm_beacon_t b2; lm_beacon_init(&b2, &bc, &saved, t); from = ntx; tick_ms(&b2, 5000);
    CHECK(count_tx(from, LM_MSG_BEACON_PAYLOAD, 3) == 1 && decode_tx(ntx - 1));
    lm_beacon_payload_t p; memcpy(&p, got, sizeof p); CHECK(p.age_s >= 30 && p.age_s <= 36);
}

static uint8_t pipe_buf[256]; static int pipe_w, pipe_r;
static void pipe_write(const uint8_t *d, size_t n) { while (n--) pipe_buf[pipe_w++ & 255] = *d++; }
static int pipe_read(void) { return pipe_r == pipe_w ? -1 : pipe_buf[pipe_r++ & 255]; }
static void test_uart(void) {
    lm_link_uart_t u; lm_link_uart_init(&u, pipe_write, pipe_read); lm_link_if_t l = lm_link_uart_as_link(&u);
    lm_heartbeat_t h = { LM_NODE_ID_ONA, 1, 7 };
    CHECK(lm_send(&l, LM_MSG_HEARTBEAT, &h, sizeof h) == 0);
    pipe_w -= 3; rx_n = 0; lm_poll(&l, on_rx, 0); CHECK(rx_n == 0);   /* frame split across polls */
    pipe_w += 3; lm_poll(&l, on_rx, 0); CHECK(rx_n == 1 && rx_id == LM_MSG_HEARTBEAT && rx_rssi == 0);
}

int main(int argc, char **argv) {
    if (argc > 1 && !strcmp(argv[1], "airtime")) {
        static const uint32_t bws[] = { 62500, 125000, 250000 };
        for (uint8_t sf = 7; sf <= 12; sf++) for (unsigned k = 0; k < 3; k++) for (uint8_t cr = 5; cr <= 8; cr += 3)
            for (uint8_t n = 6; n <= 70; n += 16) {
                lm_lora_cfg_t c = { 433000000, sf, bws[k], cr, 17 };
                printf("%u %u %u %u %u\n", sf, (unsigned)bws[k], cr, n, (unsigned)lm_lora_airtime_ms(&c, n));
            }
        return 0;
    }
    test_init(); test_rx_tx(); test_airtime(); test_slots(); test_beacon(); test_uart();
    printf("ALL OK\n");
    return 0;
}
