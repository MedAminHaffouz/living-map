/* Minimal Arduino-ESP32 API surface used by targets/<fw>/src/main.cpp, so the glue compiles on the host.
   Signatures follow arduino-esp32; behaviour is not emulated (compile check only). */
#pragma once
#include <stdint.h>
#include <stddef.h>
#define OUTPUT 0x03
#define INPUT  0x01
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);
unsigned long millis(void);
void delay(uint32_t ms);
class HardwareSerial {
public:
    void begin(unsigned long baud);
    int available(void);
    int read(void);
    size_t write(const uint8_t *buf, size_t n);
};
extern HardwareSerial Serial;
