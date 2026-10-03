#include "executor.h"
#include "lm_link.h"
static brief_t brief; static beacon_table_t table; static actions_t act; static lm_decoder_t dec;
static void on_msg(uint8_t id, const uint8_t *p, uint8_t n, void *ctx) {
    (void)ctx; uint32_t now = board_millis();
    if (id == LM_MSG_BRIEF_HEADER && n == sizeof(lm_brief_header_t)) brief_on_header(&brief, (const void *)p);
    else if (id == LM_MSG_BRIEF_STEP && n == sizeof(lm_brief_step_t)) brief_on_step(&brief, (const void *)p);
    else if (id == LM_MSG_BEACON_OBS && n == sizeof(lm_beacon_obs_t)) beacon_on_obs(&table, (const void *)p, now);
}
void app_uart_rx_byte(uint8_t b) { lm_decoder_feed(&dec, b, on_msg, 0); }
void app_init(void) { lm_decoder_init(&dec); }
void app_tick(void) { actions_tick(&act, &brief, &table, board_millis()); }
