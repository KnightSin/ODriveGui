#pragma once

#include <inttypes.h>
#include <stddef.h>

uint8_t CRC8(uint8_t* data, size_t len);

uint16_t CRC16(uint8_t* data, size_t len);

uint16_t CRC16_JSON(uint8_t* data, size_t len);
