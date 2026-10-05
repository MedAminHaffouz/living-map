#include "executor.h"
#include <string.h>
#include "lm_link.h"
#include "lm_slots.h"
#include "lm_txq.h"

/* Ra-02 433 MHz, SF7, BW125, CR4/5, 14 dBm (sync word 0x4C is fixed in the driver) */
static const lm_lora_cfg_t RADIO = { 433000000, 7, 125000, 5, 14 };

static brief_t brief; static beacon_table_t table; static actions_t act;
static lm_lora_t radio; static lm_link_if_t air; static int radio_ok; static uint32_t t_retry;
static lm_slots_t slots;
static lm_txq_msg_t qbuf[EX_TXQ]; static lm_txq_t txq;

int ex_tx(uint8_t id, const void *p, uint8_t n) { return lm_txq_push(&txq, id, p, n); }

static void on_air(uint8_t id, const uint8_t *p, uint8_t n, int8_t rssi, void *ctx) {
    (void)ctx; uint32_t now = board_millis();
    if (id == LM_MSG_BRIEF_HEADER && n == sizeof(lm_brief_header_t)) brief_on_header(&brief, (const void *)p);
    else if (id == LM_MSG_BRIEF_STEP && n == sizeof(lm_brief_step_t)) brief_on_step(&brief, (const void *)p);
    else if (id == LM_MSG_BEACON_PAYLOAD && n == sizeof(lm_beacon_payload_t)) {
        lm_beacon_obs_t o; memcpy(&o, p, n); o.rssi = rssi;                    /* wire -> wire: payload is the obs prefix */
        lm_slot_sync(&slots, o.phase_ms, lm_lora_airtime_ms(&RADIO, (uint8_t)(n + 6)), now);
        beacon_on_obs(&table, &o, now);
    }
}

static void radio_init(uint32_t now) {
    t_retry = now;
    radio_ok = lm_lora_init(&radio, board_lora(), &RADIO) == LM_LORA_OK;
    if (radio_ok) air = lm_lora_as_link(&radio);
}

void app_init(void) {
    lm_slots_init(&slots); lm_txq_init(&txq, qbuf, EX_TXQ);
    radio_init(board_millis());
}
void app_tick(void) {
    uint32_t now = board_millis();
    if (!radio_ok) { if (now - t_retry >= 1000) radio_init(now); }
    else { lm_poll(&air, on_air, 0); lm_txq_flush(&txq, &air, &slots, &RADIO, now); }
    actions_tick(&act, &brief, &table, now);
}
