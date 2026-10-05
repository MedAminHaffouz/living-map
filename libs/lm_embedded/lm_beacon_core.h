/* Board-agnostic beacon: stores one BeaconPayload, acks writes, broadcasts once per TDMA period in its slot.
   The board main only builds the link (lm_lora_as_link) and calls lm_beacon_tick() in its loop. */
#pragma once
#include <stdint.h>
#include "lm_msgs.h"
#include "lm_link_if.h"
#include "lm_slots.h"

/* BeaconPayload.flags */
#define LM_BEACON_FLAG_SUSPECT     0x01
#define LM_BEACON_FLAG_VERIFIED    0x02
#define LM_BEACON_FLAG_ACTION_DONE 0x04

typedef struct {
    uint8_t id;
    const lm_link_if_t *link;
    uint32_t airtime_ms;   /* time on air of a BeaconPayload frame on this link (lm_lora_airtime_ms), for slot sync */
    void (*persist)(const lm_beacon_payload_t *p, void *ctx);   /* optional: called on every accepted write */
    void *persist_ctx;
} lm_beacon_cfg_t;

typedef struct {
    lm_beacon_cfg_t cfg;
    lm_slots_t slots;
    lm_beacon_payload_t pl;
    uint8_t written, sent_in_slot, poll_pending, poll_slot;
    uint32_t written_at, now;
} lm_beacon_t;

/* restored = payload from persist storage (NULL if none); its age_s is carried over */
void lm_beacon_init(lm_beacon_t *b, const lm_beacon_cfg_t *cfg, const lm_beacon_payload_t *restored, uint32_t now_ms);
void lm_beacon_tick(lm_beacon_t *b, uint32_t now_ms);
