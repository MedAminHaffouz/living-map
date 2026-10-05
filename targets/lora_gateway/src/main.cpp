// lora_gateway: board glue only (Ra-02 on VSPI, USB serial, millis). Bridge logic is in gateway.c.
#include <Arduino.h>
#include <SPI.h>
extern "C" {
#include "lm_lora_sx127x.h"
#include "lm_link_uart.h"
#include "gateway.h"
}

// ESP32 DevKit (esp32dev) <-> Ra-02
static const int PIN_SCK = 18, PIN_MISO = 19, PIN_MOSI = 23, PIN_NSS = 5, PIN_RST = 14, PIN_DIO0 = 26;
static const uint32_t USB_BAUD = 921600;

static uint8_t  spi_xfer(uint8_t b) { return SPI.transfer(b); }
static void     nss(int level)      { digitalWrite(PIN_NSS, level); }
static void     rst(int level)      { digitalWrite(PIN_RST, level); }
static int      dio0(void)          { return digitalRead(PIN_DIO0); }
static uint32_t ms(void)            { return millis(); }
static const lm_lora_board_t BOARD = { spi_xfer, nss, rst, dio0, ms };
static const lm_lora_cfg_t RADIO = LM_LORA_CFG_DEFAULT;

static void usb_write(const uint8_t *d, size_t n) { Serial.write(d, n); }
static int  usb_read(void) { return Serial.available() ? Serial.read() : -1; }

static lm_lora_t radio;
static lm_link_uart_t usb;
static lm_link_if_t air, host;
static gateway_t gw;

void setup() {
    Serial.begin(USB_BAUD);
    pinMode(PIN_NSS, OUTPUT); pinMode(PIN_RST, OUTPUT); pinMode(PIN_DIO0, INPUT);
    SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_NSS);
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));   // only device on the bus: never released
    while (lm_lora_init(&radio, &BOARD, &RADIO) != LM_LORA_OK) delay(1000);
    air = lm_lora_as_link(&radio);
    lm_link_uart_init(&usb, usb_write, usb_read);
    host = lm_link_uart_as_link(&usb);
    gateway_init(&gw, &air, &host, &RADIO);
}

void loop() { gateway_tick(&gw, millis()); }
