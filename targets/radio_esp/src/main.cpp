// radio_esp: transparent bridge  host (UART/USB, lm_link frames)  <->  ESP-NOW broadcast (same frames).
// Air -> host: BeaconPayload broadcasts are re-wrapped as BeaconObs with the packet RSSI.
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
extern "C" {
#include "lm_link.h"
#include "lm_msgs.h"
}
static const uint8_t BCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static lm_decoder_t dec;
static HardwareSerial &HOST = Serial;   // TODO: Serial2 for the Executor STM UART

static void to_host(uint8_t id, const void *p, uint8_t n) { uint8_t f[LM_MAX_PAYLOAD + 6]; HOST.write(f, lm_encode(id, p, n, f)); }

static void on_air(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    // each ESP-NOW packet carries exactly one lm_link frame
    lm_decoder_t d; lm_decoder_init(&d);
    int8_t rssi = info->rx_ctrl ? info->rx_ctrl->rssi : 0;
    for (int i = 0; i < len; i++)
        lm_decoder_feed(&d, data[i], [](uint8_t id, const uint8_t *p, uint8_t n, void *ctx) {
            int8_t r = *(int8_t *)ctx;
            if (id == LM_MSG_BEACON_PAYLOAD && n == sizeof(lm_beacon_payload_t)) {
                lm_beacon_obs_t o; memcpy(&o, p, sizeof(lm_beacon_payload_t)); o.rssi = r;
                to_host(LM_MSG_BEACON_OBS, &o, sizeof o);
            } else to_host(id, p, n);                       // acks, brief, reports: pass through
        }, &rssi);
}
static void on_host(uint8_t id, const uint8_t *p, uint8_t n, void *) {
    uint8_t f[LM_MAX_PAYLOAD + 6]; size_t k = lm_encode(id, p, n, f);
    esp_now_send(BCAST, f, k);                             // TODO: retry until BeaconAck for BeaconPayload
}
void setup() {
    HOST.begin(921600); WiFi.mode(WIFI_STA); esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    esp_now_init(); esp_now_register_recv_cb(on_air);
    esp_now_peer_info_t peer = {}; memcpy(peer.peer_addr, BCAST, 6); peer.channel = 1; esp_now_add_peer(&peer);
    lm_decoder_init(&dec);
}
void loop() { while (HOST.available()) lm_decoder_feed(&dec, HOST.read(), on_host, nullptr); }
