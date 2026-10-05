/* Host-side cross-check: decode Python-encoded frames with the C decoder + generated structs. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lm_link.h"
#include "lm_aging.h"
#include "lm_msgs.h"

static void on_msg(uint8_t id, const uint8_t *p, uint8_t len, void *ctx) {
    (void)ctx;
    if (id == LM_MSG_SENSOR_DET && len == sizeof(lm_sensor_det_t)) {
        lm_sensor_det_t m; memcpy(&m, p, len);
        printf("SensorDet type=%u detected=%u value=%.1f conf=%u t_ms=%u\n", m.type, m.detected, m.value, m.conf, m.t_ms);
    } else if (id == LM_MSG_BEACON_PAYLOAD && len == sizeof(lm_beacon_payload_t)) {
        lm_beacon_payload_t m; memcpy(&m, p, len);
        printf("BeaconPayload id=%u what=%u prio=%u dir=%u dist=%u age=%u ver=%u phase=%u state=%u\n",
               m.id, m.what, m.prio, m.dir_deg, m.dist_cm, m.age_s, m.version, m.phase_ms,
               lm_age_state(m.conf / 255.f, m.age_s, m.what, m.flags));
    } else printf("unknown id=0x%02X len=%u\n", id, len);
}
int main(int argc, char **argv) {
    lm_decoder_t d; lm_decoder_init(&d);
    for (int a = 1; a < argc; a++)
        for (size_t i = 0; i + 1 < strlen(argv[a]); i += 2) {
            char h[3] = { argv[a][i], argv[a][i + 1], 0 };
            lm_decoder_feed(&d, (uint8_t)strtol(h, NULL, 16), on_msg, NULL);
        }
    return 0;
}
