#pragma once

// TODO: this file assumes the processor is little-endian. We should create
// another no-op implementation for big-endian processors!

#include <assert.h>

static inline uint16_t swap(uint16_t val) {
  return (val >> 8) | (val << 8);
}

static inline uint32_t swap(uint32_t val) {
  return ((val >> 24) & 0xFF)
    | ((val >> 8) & 0xFF00)
    | ((val << 8) & 0xFF0000)
    | ((val << 24) & 0xFF000000);
}

static inline uint16_t hton(uint16_t val) {
  return swap(val);
}

static inline uint32_t hton(uint32_t val) {
  return swap(val);
}

static inline uint16_t ntoh(uint16_t val) {
  return swap(val);
}

static inline uint32_t ntoh(uint32_t val) {
  return swap(val);
}
