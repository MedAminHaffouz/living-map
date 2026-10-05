/* Byte-stream transport (UART, USB-CDC) as an lm_link_if_t. rssi is always 0. */
#pragma once
#include <stddef.h>
#include "lm_link.h"
#include "lm_link_if.h"

typedef struct {
    void (*write)(const uint8_t *d, size_t n);
    int  (*read_byte)(void);                     /* next received byte, or -1 if none */
    lm_decoder_t dec;
} lm_link_uart_t;

void lm_link_uart_init(lm_link_uart_t *u, void (*write)(const uint8_t *, size_t), int (*read_byte)(void));
lm_link_if_t lm_link_uart_as_link(lm_link_uart_t *u);
