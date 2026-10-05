#include "executor.h"
void action_report_send(uint8_t beacon_id, uint8_t action, uint8_t result, uint16_t mid, uint32_t now) {
    lm_action_report_t r = { mid, beacon_id, action, result, now / 1000 };
    ex_tx(LM_MSG_ACTION_REPORT, &r, sizeof r);   /* LoRa broadcast; ONA gateway hears it when in range */
}
void beacon_update_send(const beacon_entry_t *e, uint8_t flags, uint8_t action, uint8_t result, uint16_t mid, uint32_t now) {
    if (e) { lm_beacon_payload_t p = e->p; p.version++; p.age_s = 0; p.flags = flags; ex_tx(LM_MSG_BEACON_PAYLOAD, &p, sizeof p); }
    action_report_send(e ? e->p.id : 0xFF, action, result, mid, now);
}
