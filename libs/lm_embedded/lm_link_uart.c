#include "lm_link_uart.h"

typedef struct { lm_link_rx_cb cb; void *user; } rx_ctx_t;
static void on_frame(uint8_t id, const uint8_t *p, uint8_t len, void *ctx) {
    const rx_ctx_t *r = ctx; r->cb(id, p, len, 0, r->user);
}
static int uart_send(void *ctx, uint8_t id, const void *payload, uint8_t len) {
    lm_link_uart_t *u = ctx; uint8_t f[LM_MAX_PAYLOAD + 6];
    if (len > LM_MAX_PAYLOAD) return -1;
    u->write(f, lm_encode(id, payload, len, f));
    return 0;
}
static void uart_poll(void *ctx, lm_link_rx_cb cb, void *user) {
    lm_link_uart_t *u = ctx; rx_ctx_t r = { cb, user }; int c;
    while ((c = u->read_byte()) >= 0) lm_decoder_feed(&u->dec, (uint8_t)c, on_frame, &r);   /* decoder state spans polls */
}
void lm_link_uart_init(lm_link_uart_t *u, void (*write)(const uint8_t *, size_t), int (*read_byte)(void)) {
    u->write = write; u->read_byte = read_byte; lm_decoder_init(&u->dec);
}
lm_link_if_t lm_link_uart_as_link(lm_link_uart_t *u) { return (lm_link_if_t){ u, uart_send, uart_poll }; }
