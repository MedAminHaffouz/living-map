#include "executor.h"
/* Fire Action Node: pump bursts, hard timeout on the STM side (pump never left on). */
#define BURST_MS 2000
static uint32_t t0, left_ms = 20000;   /* extinguisher capacity, TODO measure */
static int precond(const beacon_entry_t *e) { return e && left_ms >= BURST_MS; }
static void start(const beacon_entry_t *e, uint32_t now) { (void)e; t0 = now; board_pump(1); }
static act_status_t tick(uint32_t now) {
    if (now - t0 < BURST_MS) return ACT_RUNNING;
    board_pump(0); left_ms -= BURST_MS; return ACT_OK;
}
static int verify(void) { return board_read_temp_c() < 45.f; }   /* fire out? */
const actuator_node_t FIRE_ACTION_NODE = { LM_ACTION_EXTINGUISH, precond, start, tick, verify };
