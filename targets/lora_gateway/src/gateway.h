/* LoRa <-> USB gateway logic (board-agnostic; main.cpp is the glue).
   Air -> host: BeaconPayload becomes BeaconObs + packet RSSI (and syncs our TDMA phase); everything else passes through.
   Host -> air: queued (GW_QUEUE entries) and transmitted only inside the reader window (slot 24). */
#pragma once
#include "lm_link.h"
#include "lm_link_if.h"
#include "lm_lora_sx127x.h"
#include "lm_slots.h"

#define GW_QUEUE 16

typedef struct { uint8_t id, len; uint8_t p[LM_MAX_PAYLOAD]; } gw_msg_t;
typedef struct {
    const lm_link_if_t *air, *host;
    const lm_lora_cfg_t *radio;
    lm_slots_t slots;
    gw_msg_t q[GW_QUEUE]; uint8_t head, n;
    uint32_t dropped, now;
} gateway_t;

void gateway_init(gateway_t *g, const lm_link_if_t *air, const lm_link_if_t *host, const lm_lora_cfg_t *radio);
void gateway_tick(gateway_t *g, uint32_t now_ms);
