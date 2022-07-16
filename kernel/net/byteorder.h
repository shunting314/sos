#pragma once

// TODO: this file assumes the processor is little-endian. We should create
// another no-op implementation for big-endian processors!

#include <assert.h>

static inline uint16_t hton(uint16_t val) {
  return (val >> 8) | (val << 8);
}
