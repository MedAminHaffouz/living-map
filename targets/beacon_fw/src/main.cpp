// beacon_fw: board glue only (Ra-02 on SPI, millis, NVS). All beacon behaviour is in libs/lm_embedded/lm_beacon_core.c.
#include <Arduino.h>
#include <SPI.h>
#include <Preferences.h>
extern "C" {
#include "lm_lora_sx127x.h"
#include "lm_beacon_core.h"
}
#ifndef BEACON_ID
#error "build with -DBEACON_ID=n"
#endif

// ESP32-C3 DevKitM-1 <-> Ra-02
static const int PIN_SCK = 4, PIN_MISO = 5, PIN_MOSI = 6, PIN_NSS = 7, PIN_RST = 3, PIN_DIO0 = 2;

static uint8_t  spi_xfer(uint8_t b) { return SPI.transfer(b); }
static void     nss(int level)      { digitalWrite(PIN_NSS, level); }
static void     rst(int level)      { digitalWrite(PIN_RST, level); }
static int      dio0(void)          { return digitalRead(PIN_DIO0); }
static uint32_t ms(void)            { return millis(); }
static const lm_lora_board_t BOARD = { spi_xfer, nss, rst, dio0, ms };
static const lm_lora_cfg_t RADIO = LM_LORA_CFG_DEFAULT;

static Preferences nvs;
static lm_lora_t radio;
static lm_link_if_t air;
static lm_beacon_t beacon;

static void persist(const lm_beacon_payload_t *p, void *) { nvs.putBytes("pl", p, sizeof *p); }

void setup() {
    pinMode(PIN_NSS, OUTPUT); pinMode(PIN_RST, OUTPUT); pinMode(PIN_DIO0, INPUT);
    SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_NSS);
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));   // only device on the bus: never released
    while (lm_lora_init(&radio, &BOARD, &RADIO) != LM_LORA_OK) delay(1000);
    air = lm_lora_as_link(&radio);

    nvs.begin("lm", false);
    lm_beacon_payload_t saved;
    bool have = nvs.getBytesLength("pl") == sizeof saved && nvs.getBytes("pl", &saved, sizeof saved) == sizeof saved
                && saved.id == BEACON_ID;
    if (have) saved.flags |= LM_BEACON_FLAG_SUSPECT;   // rebooted: the age restarts, readers must not trust it
    lm_beacon_cfg_t cfg = { BEACON_ID, &air, lm_lora_airtime_ms(&RADIO, sizeof(lm_beacon_payload_t) + 6), persist, nullptr };
    lm_beacon_init(&beacon, &cfg, have ? &saved : nullptr, millis());
}

void loop() { lm_beacon_tick(&beacon, millis()); }
