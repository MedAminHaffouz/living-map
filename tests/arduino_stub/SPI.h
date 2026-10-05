#pragma once
#include <stdint.h>
#define MSBFIRST 1
#define SPI_MODE0 0
class SPISettings { public: SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode); };
class SPIClass {
public:
    void begin(int8_t sck = -1, int8_t miso = -1, int8_t mosi = -1, int8_t ss = -1);
    void beginTransaction(SPISettings settings);
    uint8_t transfer(uint8_t data);
};
extern SPIClass SPI;
