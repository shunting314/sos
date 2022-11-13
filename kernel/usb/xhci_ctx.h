#pragma once

#include <string.h>

class SlotContext {
 private:
  uint32_t dummy[8];
};

static_assert(sizeof(SlotContext) == 32);

class EndpointContext {
 private:
  uint32_t dummy[8];
};

static_assert(sizeof(SlotContext) == 32);


// xhci spec 6.2.1: all unused entries of the device context shall be
// initialized to 0 by software.
class DeviceContext {
 public:
  explicit DeviceContext() {
    memset(ctx_list_, 0, sizeof(ctx_list_));
  }
 private:
  // the first one is actually a slot context. Organize this way so there is
  // no off by 1 operation needed to calculate the Device Context Index.
  EndpointContext ctx_list_[32];
} __attribute__((aligned(64)));

static_assert(sizeof(DeviceContext) == 1024);
