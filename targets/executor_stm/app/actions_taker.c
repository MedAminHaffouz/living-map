#include "executor.h"
#include "lm_aging.h"
/* Per step: NAVIGATE to beacon -> (stale? VERIFY) -> ACT via actuator node -> REPORT (Beacon Update) -> next.
   Trust by age: FRESH/AGING act directly, STALE verify first, SUSPECT skip + report. */
static const actuator_node_t *node_for(uint8_t action) {
    switch (action) { case LM_ACTION_EXTINGUISH: return &FIRE_ACTION_NODE; case LM_ACTION_FIRST_AID: return &FIRST_AID_NODE; default: return 0; }
}
static void go(actions_t *a, ex_state_t s, uint32_t now) { a->st = s; a->t_state = now; }

void actions_tick(actions_t *a, brief_t *b, beacon_table_t *t, uint32_t now) {
    if (a->st == EX_IDLE) { if (brief_complete(b)) go(a, EX_NAVIGATE, now); return; }
    if (a->st == EX_DONE) return;
    if (a->idx >= b->n) { go(a, EX_DONE, now); return; }
    const lm_brief_step_t *s = &b->step[a->idx];
    const beacon_entry_t *e = beacon_get(t, s->beacon_id);
    const actuator_node_t *n = node_for(s->action);
    switch (a->st) {
    case EX_NAVIGATE: {
        nav_status_t ns = nav_to_beacon(t, s->beacon_id, now);
        if (ns == NAV_LOST) { beacon_update_send(e, 0, s->action, LM_RESULT_BLOCKED, b->mission_id, now); a->idx++; break; }
        if (ns == NAV_ARRIVED) {
            if (!e || e->state == LM_AGE_SUSPECT) { a->idx++; break; }
            go(a, e->state == LM_AGE_STALE ? EX_VERIFY : (n ? EX_ACT : EX_REPORT), now);
            if (a->st == EX_ACT && n->precond(e)) n->start(e, now);
        } break; }
    case EX_VERIFY:   /* TODO: own-sensor check of e->p.what; contradicted -> flags suspect */
        go(a, n ? EX_ACT : EX_REPORT, now); if (n && n->precond(e)) n->start(e, now); break;
    case EX_ACT: {
        act_status_t r = n->tick(now);
        if (r == ACT_RUNNING) break;
        int ok = (r == ACT_OK) && n->verify();
        if (!ok && ++a->tries < 3) { n->start(e, now); break; }
        beacon_update_send(e, ok ? 0x06 : 0x00, s->action, ok ? LM_RESULT_OK : LM_RESULT_FAIL, b->mission_id, now);
        a->tries = 0; a->idx++; go(a, EX_NAVIGATE, now); break; }
    case EX_REPORT:
        beacon_update_send(e, 0x02, s->action, LM_RESULT_OK, b->mission_id, now); a->idx++; go(a, EX_NAVIGATE, now); break;
    default: break;
    }
}
