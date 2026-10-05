/* LoRa TDMA: 5000 ms period, 25 slots of 200 ms. Beacon i talks in slot i % 24, slot 24 is the reader window
   (Writer / Executor / ONA). No common clock: every node keeps a phase offset and nudges it toward the phase
   carried by the packets it hears (BeaconPayload.phase_ms). */
#pragma once
#include <stdint.h>

#define LM_SLOT_PERIOD_MS 5000
#define LM_SLOT_MS        200
#define LM_SLOT_COUNT     25
#define LM_SLOT_READER    24

typedef struct { int32_t offset_ms; uint8_t synced; } lm_slots_t;

void     lm_slots_init(lm_slots_t *s);
uint16_t lm_slot_phase(const lm_slots_t *s, uint32_t now_ms);          /* 0..PERIOD-1 */
uint8_t  lm_slot_current(const lm_slots_t *s, uint32_t now_ms);        /* 0..COUNT-1 */
static inline uint8_t lm_slot_of_beacon(uint8_t id) { return (uint8_t)(id % LM_SLOT_READER); }
/* rx_phase = sender's phase when it started sending, airtime = that packet's time on air, now = reception time.
   First call jumps to the sender's phase, later calls correct by err/4 (err wrapped to +-PERIOD/2). */
void     lm_slot_sync(lm_slots_t *s, uint16_t rx_phase, uint32_t airtime_ms, uint32_t now_ms);
