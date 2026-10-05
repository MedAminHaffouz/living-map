/* Executor brief walk per robot type: the matching step is acted on (beacon v+1 + ActionReport OK), the other one is
   reported ABORTED without touching its beacon or the actuator. Links every app source except app.c (radio glue). */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "executor.h"

#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); exit(1); } } while (0)

static int pumps, servos;
uint32_t board_millis(void) { return 0; }
const lm_lora_board_t *board_lora(void) { return 0; }
void  board_pump(int on) { pumps += on; }
void  board_kit_servo_release(void) { servos++; }
int   board_kit_bay_empty(void) { return 1; }
float board_read_temp_c(void) { return 25.f; }
void  board_drive(float v, float w) { (void)v; (void)w; }

typedef struct { uint8_t id, n, p[LM_MAX_PAYLOAD]; } tx_t;
static tx_t tx[16]; static int ntx;
int ex_tx(uint8_t id, const void *p, uint8_t n) { CHECK(ntx < 16); tx[ntx].id = id; tx[ntx].n = n; memcpy(tx[ntx++].p, p, n); return 0; }

static lm_action_report_t report(int i) { lm_action_report_t r; CHECK(tx[i].id == LM_MSG_ACTION_REPORT); memcpy(&r, tx[i].p, sizeof r); return r; }
static lm_beacon_payload_t write_of(int i) { lm_beacon_payload_t p; CHECK(tx[i].id == LM_MSG_BEACON_PAYLOAD); memcpy(&p, tx[i].p, sizeof p); return p; }

int main(void) {
    brief_t b; beacon_table_t t; actions_t a; memset(&t, 0, sizeof t); memset(&a, 0, sizeof a);
    lm_brief_header_t h = { 9, 2, 0 }; brief_on_header(&b, &h);
    lm_brief_step_t s0 = { 9, 0, 2, LM_ACTION_EXTINGUISH, LM_EVENT_TYPE_FIRE }, s1 = { 9, 1, 3, LM_ACTION_FIRST_AID, LM_EVENT_TYPE_PERSON };
    brief_on_step(&b, &s0); brief_on_step(&b, &s1); CHECK(brief_complete(&b));
    lm_beacon_obs_t o2 = { .id = 2, .what = LM_EVENT_TYPE_FIRE, .conf = 230, .age_s = 5, .rssi = -50 };
    lm_beacon_obs_t o3 = { .id = 3, .what = LM_EVENT_TYPE_PERSON, .conf = 230, .age_s = 5, .rssi = -50 };

    for (uint32_t now = 1000; now < 30000 && a.st != EX_DONE; now += 10) {
        beacon_on_obs(&t, &o2, now); beacon_on_obs(&t, &o3, now);     /* both beacons in range, fresh */
        actions_tick(&a, &b, &t, now);
    }
    CHECK(a.st == EX_DONE && ntx == 3);
#if EXECUTOR_TYPE == EX_FIRE
    CHECK(ex_actuator() == &FIRE_ACTION_NODE && pumps >= 1 && servos == 0);
    lm_beacon_payload_t w = write_of(0); CHECK(w.id == 2 && w.version == 1 && w.age_s == 0 && w.flags == 0x06);
    lm_action_report_t r = report(1); CHECK(r.beacon_id == 2 && r.action == LM_ACTION_EXTINGUISH && r.result == LM_RESULT_OK && r.mission_id == 9);
    r = report(2); CHECK(r.beacon_id == 3 && r.action == LM_ACTION_FIRST_AID && r.result == LM_RESULT_ABORTED);
#else
    CHECK(ex_actuator() == &FIRST_AID_NODE && pumps == 0 && servos == 1);
    lm_action_report_t r = report(0); CHECK(r.beacon_id == 2 && r.action == LM_ACTION_EXTINGUISH && r.result == LM_RESULT_ABORTED);
    lm_beacon_payload_t w = write_of(1); CHECK(w.id == 3 && w.version == 1 && w.age_s == 0 && w.flags == 0x06);
    r = report(2); CHECK(r.beacon_id == 3 && r.action == LM_ACTION_FIRST_AID && r.result == LM_RESULT_OK && r.mission_id == 9);
#endif
    printf("ALL OK\n");
    return 0;
}
