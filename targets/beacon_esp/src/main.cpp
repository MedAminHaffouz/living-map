// beacon_esp: the Living Map node. Boxes: Events Data + Map Data (payload), Aging Mechanism, Message coms, Next pointer.
// - accepts BeaconPayload for its own id with version >= stored (Writer write, Executor update)
// - counts its own age (no clock sync): age_s = (now - written_at) / 1000
// - broadcasts payload every 500 ms (radio ESPs add RSSI on reception)
// - TODO relay: re-broadcast ActionReport / foreign beacons toward B0 with TTL (BEACONS COMS -> ONA)
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Preferences.h>
extern "C" {
#include "lm_link.h"
#include "lm_msgs.h"
#include "lm_aging.h"
}
static const uint8_t BCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static lm_beacon_payload_t pl = {}; static bool written = false; static uint32_t written_at = 0;
static Preferences nvs;

static void air_send(uint8_t id, const void *p, uint8_t n) { uint8_t f[LM_MAX_PAYLOAD + 6]; esp_now_send(BCAST, f, lm_encode(id, p, n, f)); }

static void on_air(const esp_now_recv_info_t *, const uint8_t *data, int len) {
    lm_decoder_t d; lm_decoder_init(&d);
    for (int i = 0; i < len; i++)
        lm_decoder_feed(&d, data[i], [](uint8_t id, const uint8_t *p, uint8_t n, void *) {
            if (id != LM_MSG_BEACON_PAYLOAD || n != sizeof(lm_beacon_payload_t)) return;
            const lm_beacon_payload_t *in = (const lm_beacon_payload_t *)p;
            if (in->id != BEACON_ID || in->age_s != 0) return;                  // age_s==0 marks a write, not a broadcast
            bool ok = !written || in->version >= pl.version;
            if (ok) { pl = *in; written = true; written_at = millis(); nvs.putBytes("pl", &pl, sizeof pl); }
            lm_beacon_ack_t a = { (uint8_t)BEACON_ID, in->version, (uint8_t)ok }; air_send(LM_MSG_BEACON_ACK, &a, sizeof a);
        }, nullptr);
}
void setup() {
    WiFi.mode(WIFI_STA); esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    esp_now_init(); esp_now_register_recv_cb(on_air);
    esp_now_peer_info_t peer = {}; memcpy(peer.peer_addr, BCAST, 6); peer.channel = 1; esp_now_add_peer(&peer);
    nvs.begin("lm", false);   // survives brown-out; age restarts (honest: flag it) TODO
}
void loop() {
    static uint32_t t = 0;
    if (written && millis() - t >= 500) {
        t = millis(); lm_beacon_payload_t out = pl;
        out.age_s = (millis() - written_at) / 1000; if (out.age_s == 0) out.age_s = 1;
        air_send(LM_MSG_BEACON_PAYLOAD, &out, sizeof out);
    }
    delay(5);
}
