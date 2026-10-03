/* DropCmd -> servo magazine releases slot. Ack = next Heartbeat state bit (TODO limit switch). */
#pragma once
#include <stdint.h>
void dropper_release(uint8_t slot);
