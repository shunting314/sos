#pragma once

#include <stdint.h>

uint32_t crc32(const char *hex);
uint32_t crc32(const uint8_t* data, int len);
