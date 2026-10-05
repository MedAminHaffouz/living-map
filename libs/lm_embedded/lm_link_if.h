/* Transport-agnostic link: firmware sends/receives lm_link messages only through this (LoRa, UART, ...). */
#pragma once
#include <stdint.h>

/* rssi in dBm for radio links, 0 for wired ones */
typedef void (*lm_link_rx_cb)(uint8_t id, const uint8_t *payload, uint8_t len, int8_t rssi, void *user);

typedef struct lm_link_if_t {
    void *ctx;
    int  (*send)(void *ctx, uint8_t id, const void *payload, uint8_t len);   /* 0 = sent, <0 = error */
    void (*poll)(void *ctx, lm_link_rx_cb cb, void *user);                   /* non-blocking; cb per valid frame */
} lm_link_if_t;

static inline int lm_send(const lm_link_if_t *l, uint8_t id, const void *payload, uint8_t len) {
    return l->send(l->ctx, id, payload, len);
}
static inline void lm_poll(const lm_link_if_t *l, lm_link_rx_cb cb, void *user) { l->poll(l->ctx, cb, user); }
