#include "executor.h"
#include <stddef.h>
#include <string.h>
#include "lm_aging.h"
_Static_assert(offsetof(lm_beacon_obs_t, rssi) == sizeof(lm_beacon_payload_t), "BeaconObs = BeaconPayload + rssi");
void beacon_on_obs(beacon_table_t *t, const lm_beacon_obs_t *o, uint32_t now) {
    if (o->id >= EX_MAX_BEACONS) return;
    beacon_entry_t *e = &t->b[o->id];
    if (e->valid && o->version < e->p.version) return;                 /* older version: ignore */
    memcpy(&e->p, o, sizeof e->p);                                       /* wire -> wire: payload is the obs prefix */
    e->rssi = o->rssi; e->heard_ms = now; e->valid = 1;
    e->state = lm_age_state(o->conf / 255.f, o->age_s, o->what, o->flags);  /* fresh / aging / stale / suspect */
}
const beacon_entry_t *beacon_get(const beacon_table_t *t, uint8_t id) { return (id < EX_MAX_BEACONS && t->b[id].valid) ? &t->b[id] : 0; }
