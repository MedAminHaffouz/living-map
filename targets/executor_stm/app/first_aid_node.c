#include "executor.h"
/* First Aid Node: release one kit near (never on) the person. */
static uint32_t t0; static uint8_t kits = 3;
static int precond(const beacon_entry_t *e) { return e && kits > 0; }
static void start(const beacon_entry_t *e, uint32_t now) { (void)e; t0 = now; board_kit_servo_release(); }
static act_status_t tick(uint32_t now) { return (now - t0 < 800) ? ACT_RUNNING : (kits--, ACT_OK); }
static int verify(void) { return 1; /* TODO: bay switch: board_kit_bay_empty() */ }
const actuator_node_t FIRST_AID_NODE = { LM_ACTION_FIRST_AID, precond, start, tick, verify };
