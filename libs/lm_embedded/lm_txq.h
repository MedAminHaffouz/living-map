/* Outgoing LoRa queue for readers (Executor, ONA gateway): messages wait for the reader window (slot 24)
   and only go out if the packet ends before the window does. */
#pragma once
#include <stdint.h>
#include "lm_link.h"
#include "lm_link_if.h"
#include "lm_lora_sx127x.h"
#include "lm_slots.h"

typedef struct { uint8_t id, len; uint8_t p[LM_MAX_PAYLOAD]; } lm_txq_msg_t;
typedef struct { lm_txq_msg_t *buf; uint8_t cap, head, n; uint32_t dropped; } lm_txq_t;

void lm_txq_init(lm_txq_t *q, lm_txq_msg_t *buf, uint8_t cap);
/* 0 = queued, -1 = full (dropped and counted: what is already queued keeps its order) or too long */
int  lm_txq_push(lm_txq_t *q, uint8_t id, const void *p, uint8_t len);
/* sends what fits in the current reader window; returns now advanced by the airtime spent */
uint32_t lm_txq_flush(lm_txq_t *q, const lm_link_if_t *link, const lm_slots_t *s, const lm_lora_cfg_t *radio, uint32_t now_ms);
