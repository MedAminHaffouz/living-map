#include "executor.h"
/* Beacon-to-beacon navigation. P1: RSSI gradient + arrive threshold. TODO: bearing from dir_deg, obstacle avoidance. */
#define ARRIVE_RSSI (-55)
#define LOST_MS 15000
static uint8_t cur_target = 0xFF; static uint32_t t_start;
nav_status_t nav_to_beacon(const beacon_table_t *t, uint8_t id, uint32_t now) {
    if (id != cur_target) { cur_target = id; t_start = now; }
    const beacon_entry_t *e = beacon_get(t, id);
    if (e && e->rssi >= ARRIVE_RSSI && now - e->heard_ms < 1000) { board_drive(0, 0); return NAV_ARRIVED; }
    if (now - t_start > LOST_MS) { board_drive(0, 0); return NAV_LOST; }
    board_drive(0.2f, 0.f);   /* TODO: steer by RSSI trend / dir_deg */
    return NAV_RUNNING;
}
