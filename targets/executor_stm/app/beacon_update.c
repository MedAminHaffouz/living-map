#include "executor.h"
#include "lm_link.h"
static void send(uint8_t id, const void *p, uint8_t n) { uint8_t f[LM_MAX_PAYLOAD + 6]; board_uart_write(f, lm_encode(id, p, n, f)); }
void beacon_update_send(const beacon_entry_t *e, uint8_t flags, uint8_t action, uint8_t result, uint16_t mid, uint32_t now) {
    if (e) { lm_beacon_payload_t p = e->p; p.version++; p.age_s = 0; p.flags = flags; send(LM_MSG_BEACON_PAYLOAD, &p, sizeof p); }
    lm_action_report_t r = { mid, e ? e->p.id : 0xFF, action, result, now / 1000 };
    send(LM_MSG_ACTION_REPORT, &r, sizeof r);   /* radio ESP broadcasts; ONA hears it via B0 / at exit */
}
