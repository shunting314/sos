#pragma once

#include <string.h>
#include <kernel/usb/xhci_ring.h>

class SlotContext {
 public:
  // refer to xhci spec 6.2.2 for slot state value assignment.
  const char* get_state_str() const {
    switch (slot_state) {
    case 0:
      return "DISABLED/ENABLED";
    case 1:
      return "DEFAULT";
    case 2:
      return "ADDRESSED";
    case 3:
      return "CONFIGURED";
    default:
      assert(false && "invalid slot state");
    }
    assert(false && "can not reach here");
  }
 public:
  // word 0
  uint32_t route_string : 20;
  uint32_t speed : 4;
  uint32_t rsvd : 1;
  uint32_t mtt : 1;
  uint32_t hub : 1;
  uint32_t context_entries : 5;

  // word 1
  uint32_t max_exit_latency : 16;
  uint32_t root_hub_port_number : 8;
  uint32_t number_of_ports : 8;

  // word 2
  uint32_t word2;

  // word3
  uint32_t usb_device_address : 8;
  uint32_t rsvd2 : 19;
  uint32_t slot_state : 5;

  uint32_t word_other[4];
};

static_assert(sizeof(SlotContext) == 32);

enum EndpointType {
  INVALID = 0,
  ISOCH_OUT = 1,
  BULK_OUT = 2,
  INTERRUPT_OUT = 3,
  CONTROL = 4, // bidirectional
  ISOCH_IN = 5,
  BULK_IN = 6,
  INTERRUPT_IN = 7,
};

class EndpointContext {
 public:
  void set_tr_dequeue_pointer(uint32_t addr) {
    assert((addr & 0xF) == 0);
    rsvd3 = 0;
    tr_dequeue_pointer_low_28 = (addr >> 4);
    tr_dequeue_pointer_hi = 0;
  }

  ProducerTRBRing& get_transfer_ring() {
    assert(tr_dequeue_pointer_hi == 0);
    return *(ProducerTRBRing*) (tr_dequeue_pointer_low_28 << 4);
  }

  // workd0
  uint32_t ep_state : 3;
  uint32_t rsvd0 : 5;
  uint32_t mult : 2;
  uint32_t max_pstreams : 5;
  uint32_t lsa : 1;
  uint32_t interval : 8;
  uint32_t max_esit_payload_hi : 8;

  // word1
  uint32_t rsvd : 1;
  uint32_t cerr : 2;
  uint32_t ep_type : 3;
  uint32_t rsvd2 : 1;
  uint32_t hid : 1;
  uint32_t max_burst_size : 8;
  uint32_t max_packet_size : 16;

  // word2
  uint32_t dcs : 1; // dequeue cycle state
  uint32_t rsvd3 : 3;
  uint32_t tr_dequeue_pointer_low_28 : 28;
  uint32_t tr_dequeue_pointer_hi;

  uint32_t dummy[4];
};

static_assert(sizeof(EndpointContext) == 32);

// xhci spec 6.2.1: all unused entries of the device context shall be
// initialized to 0 by software.
class DeviceContext {
 public:
  explicit DeviceContext() {
    memset(ctx_list_, 0, sizeof(ctx_list_));
  }

  SlotContext& slot_context() {
    return *(SlotContext*) &ctx_list_[0];
  }

  EndpointContext& endpoint_context(int ep_num, bool dir_in = false) {
    int idx = 0;
    if (ep_num == 0) {
      idx = 1;
    } else {
      idx = ep_num * 2 + dir_in;
    }
    return ctx_list_[idx];
  }
 private:
  // the first one is actually a slot context. Organize this way so there is
  // no off by 1 operation needed to calculate the Device Context Index.
  EndpointContext ctx_list_[32];
} __attribute__((aligned(64)));

static_assert(sizeof(DeviceContext) == 1024);

class InputControlContext {
 public:
  void include_add_context_flags(uint32_t inc_flags) {
    add_context_flags_ |= inc_flags;
  }
 private:
  uint32_t drop_context_flags_;
  uint32_t add_context_flags_;
  uint32_t rsvd[5];
  uint8_t configuration_value_;
  uint8_t interface_number_;
  uint8_t alternate_setting_;
};
static_assert(sizeof(InputControlContext) == 32);

// InputContext is pointed to by the input_context_pointer field in commands like
// AddressDevice. Make this data structure 16 bytes aligned since the low 4 bits
// of input_context_pointer are 0.
//
// Clear the datastructure to 0 in ctor according to xhci spec 4.3.3.
class InputContext {
 public:
  explicit InputContext() {
    memset(this, 0, sizeof(*this));
  }

#define SEP 0 // SEP == 1 does not work
  #if SEP
  InputControlContext& control_context() { return control_context_; }
  DeviceContext& device_context() { return device_context_; }
  #else
  InputControlContext& control_context() { return *(InputControlContext*)&ctx_list_[0]; }
  DeviceContext& device_context() { return *(DeviceContext*) &ctx_list_[1]; }
  #endif
 private:
  #if SEP // __attribute__((packed)) is somehow ignored
  InputControlContext control_context_;
  DeviceContext device_context_;
  #else
  // define as an EndpointContext array so there is no gap between
  // control_context and device_context
  EndpointContext ctx_list_[33];
  #endif
} __attribute__((aligned(16)))
#if !SEP
__attribute__((packed))
#endif
;

static_assert(sizeof(InputContext) == 1024 + 32);
