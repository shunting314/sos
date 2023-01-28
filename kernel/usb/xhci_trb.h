#pragma once

#include <kernel/usb/usb_proto.h>
#include <stdlib.h>

enum TRBType {
  NORMAL = 1,
  SETUP_STAGE = 2,
  DATA_STAGE = 3,
  STATUS_STAGE = 4,
  LINK = 6,
  NO_OP = 8,
  ENABLE_SLOT_COMMAND = 9,
  ADDRESS_DEVICE_COMMAND = 11,
  CONFIGURE_ENDPOINT_COMMAND = 12,
  TRANSFER_EVENT = 32,
  COMMAND_COMPLETION_EVENT = 33,
  PORT_STATUS_CHANGE_EVENT = 34,
  HOST_CONTROLLER_EVENT = 37,
};

static const char* trb_type_str(TRBType trb_type) {
  switch (trb_type) {
  case LINK:
    return "link";
  case NO_OP:
    return "no_op";
  case ENABLE_SLOT_COMMAND:
    return "enable_slot_command";
  case ADDRESS_DEVICE_COMMAND:
    return "address_device_command";
  case TRANSFER_EVENT:
    return "transfer_event";
  case COMMAND_COMPLETION_EVENT:
    return "command_completion_event";
  case PORT_STATUS_CHANGE_EVENT:
    return "port_status_change_event";
  case HOST_CONTROLLER_EVENT:
    return "host_controller_event";
  default:
    printf("Unknown trb_type %d\n", trb_type);
    assert(false);
  }
  assert(false && "can not reach here");
}

static const char* trb_type_str(int trb_type) {
  return trb_type_str(TRBType(trb_type));
}

class TRBTemplate;

// subclass for all TRB classes
class TRBCommon {
 public:
  // clear the TRB to 0 so subclass don't need to worry about it.
  // Assume a TRB has 16 bytes
  explicit TRBCommon() {
    memset(this, 0, 16);
  }
  TRBTemplate toTemplate() const;
};

// an empty class has size 1 rather than 0 so different objects have different
// addresses.
static_assert(sizeof(TRBCommon) == 1);

// template for a transfer request block
class TRBTemplate : public TRBCommon {
 public:
  void dump() {
    uint32_t* ar = (uint32_t*) this;
    printf("TRBTempalte:\n");
    for (int i = 0; i < 4; ++i) {
      printf("  0x%x\n", ar[i]);
    }
  }

  template <typename T>
  T expect_trb_type() const {
    if (trb_type != T::TRB_TYPE()) {
      printf("Expect %s trb, but got %s trb\n", trb_type_str(T::TRB_TYPE()), trb_type_str(trb_type));
    }
    assert(trb_type == T::TRB_TYPE());
    return *(T*) this;
  }

  template <typename T>
  T* to_trb_type() const {
    if (trb_type == T::TRB_TYPE()) {
      return (T*) this;
    } else {
      return nullptr;
    }
  }
 public:
  uint64_t parameter;
  uint32_t status;
  uint32_t c: 1; // cycle bit
  uint32_t ent: 1; // evaluate next TRB
  uint32_t others: 8;
  uint32_t trb_type: 6;
  uint32_t control: 16;
};

static_assert(sizeof(TRBTemplate) == 16);

// use for bulk in/out transfer
class NormalTRB : public TRBCommon {
 public:
  explicit NormalTRB(uint32_t _addr, uint32_t _len, bool _ioc) {
    data_buffer_pointer = _addr;
    trb_transfer_length = _len;
    ioc = _ioc;
    trb_type = NORMAL;
  }

 public:
  uint64_t data_buffer_pointer;

  // buffer size
  uint32_t trb_transfer_length : 17;
  // number of trbs remaining for this TD (excluding the current one).
  // should be 0 for the last TRB of a TD
  uint32_t td_size : 5;
  uint32_t interrupter_target : 10;

  uint32_t c : 1;
  uint32_t ent : 1; // evaluate next TRB
  uint32_t isp : 1; // interrupt on short packet
  uint32_t ns : 1; // no snoop
  uint32_t ch : 1; // chain bit
  uint32_t ioc : 1; // interrupt on completion
  uint32_t idt : 1; // immediate data
  uint32_t rsvd : 2;
  uint32_t bei : 1; // block event interrupt
  uint32_t trb_type : 6;
  uint32_t rsvd2 : 16;
};

static_assert(sizeof(NormalTRB) == 16);

class LinkTRB : public TRBCommon {
 public:
  explicit LinkTRB(uint32_t segment_addr) {
    assert((segment_addr & 0xF) == 0);  // 16 byte align
    ring_segment_pointer_low = segment_addr;
    tc = 1; // set toggle bit
    trb_type = TRBType::LINK;
  }

  static TRBType TRB_TYPE() {
    return TRBType::LINK;
  }
 public:
  uint32_t ring_segment_pointer_low;
  uint32_t ring_segment_pointer_high;
  uint32_t rsvd1 : 22;
  uint32_t interrupter_target : 10;
  // cycle bit
  uint32_t c : 1;
  // toggle cycle
  uint32_t tc : 1;
  uint32_t rsvd2 : 2;
  // chain bit
  uint32_t ch : 1;
  uint32_t ioc : 1;
  uint32_t rsvd3 : 4;
  uint32_t trb_type : 6;
  uint32_t rsvd4 : 16;
};

static_assert(sizeof(LinkTRB) == 16);

class EnableSlotCommandTRB : public TRBCommon {
 public:
  // the caller need set the c bit properly
  explicit EnableSlotCommandTRB() {
    trb_type = ENABLE_SLOT_COMMAND;
  }
 public:
  uint32_t rsvd0;
  uint32_t rsvd1;
  uint32_t rsvd2;
  // cycle bit
  uint32_t c : 1;
  uint32_t rsvd3 : 9;
  uint32_t trb_type : 6;
  // 0 for most of the time
  uint32_t slot_type : 5;
  uint32_t rsvd4 : 11;
};

static_assert(sizeof(EnableSlotCommandTRB) == 16);

class AddressDeviceCommandTRB : public TRBCommon {
 public:
  explicit AddressDeviceCommandTRB(uint64_t _input_context_pointer, uint32_t _slot_id)
    : input_context_pointer(_input_context_pointer), slot_id(_slot_id) {
    assert((input_context_pointer & 0xF) == 0 && "Requirs 16 bytes alignment");
    trb_type = ADDRESS_DEVICE_COMMAND;
  }
 public:
  uint64_t input_context_pointer;
  uint32_t rsvd;
  uint32_t c : 1;
  uint32_t rsvd2 : 8;
  uint32_t bsr : 1; // block set address request
  uint32_t trb_type : 6;
  uint32_t rsvd3 : 8;
  uint32_t slot_id : 8;
};

static_assert(sizeof(AddressDeviceCommandTRB) == 16);

class ConfigureEndpointCommandTRB : public TRBCommon {
 public:
  explicit ConfigureEndpointCommandTRB(uint64_t _input_context_pointer, uint32_t _slot_id)
    : input_context_pointer(_input_context_pointer),
      slot_id(_slot_id) {
    assert((input_context_pointer & 0xF) == 0 && "Requirs 16 bytes alignment");
    trb_type = CONFIGURE_ENDPOINT_COMMAND;
  }
 public:
  uint64_t input_context_pointer;
  uint32_t rsvd;
  uint32_t c : 1;
  uint32_t rsvd2 : 8;
  uint32_t dc : 1; // deconfigure
  uint32_t trb_type : 6;
  uint32_t rsvd3 : 8;
  uint32_t slot_id : 8;
};

static_assert(sizeof(ConfigureEndpointCommandTRB) == 16);

class NoOpTRB : public TRBCommon {
 public:
  explicit NoOpTRB() {
    trb_type = NO_OP;
  }
 public:
  uint32_t rsvd[3];
  uint32_t c : 1;
  uint32_t rsvd2 : 9;
  uint32_t trb_type : 6;
  uint32_t rsvd3: 16;
};

static_assert(sizeof(NoOpTRB) == 16);

class SetupStageTRB : public TRBCommon {
 public:
  explicit SetupStageTRB(DeviceRequest *req) : trb_transfer_length(8), idt(1), trb_type(SETUP_STAGE) {
    bmRequestType = req->bmRequestType;
    bRequest = req->bRequest;
    wValue = req->wValue;
    wIndex = req->wIndex;
    wLength = req->wLength;
    trt = 3;
  }

 public:
  // word 0
  uint32_t bmRequestType : 8;
  uint32_t bRequest : 8;
  uint32_t wValue : 16;

  // word 1
  uint32_t wIndex : 16;
  uint32_t wLength : 16;

  // word 2
  // always 8 according to xhci spec 6.4.1.2.1
  uint32_t trb_transfer_length : 17;
  uint32_t rsvd : 5;
  uint32_t interrupt_target : 10;

  // word 3
  uint32_t c : 1;
  uint32_t rsvd2 : 4;
  uint32_t ioc : 1;
  // immediate data. This bit shall be set to 1 in a Setup Stage TRB.
  // It specifies that the Parameter component of this TRB contains Setup Data
  uint32_t idt : 1;
  uint32_t rsvd3 : 3;
  uint32_t trb_type : 6;
  // transfer type
  // 0: No data stage
  // 1: reserved
  // 2: OUT data stage
  // 3: IN data stage
  uint32_t trt : 2;
  uint32_t rsvd4 : 14;
};

static_assert(sizeof(SetupStageTRB) == 16);

class DataStageTRB : public TRBCommon {
 public:
  explicit DataStageTRB(uint32_t addr, uint32_t len, bool has_more, int nremaining_trb, bool is_dir_in) : trb_type(DATA_STAGE) {
    data_buffer_lo = addr;
    data_buffer_hi = 0;

    trb_transfer_length = len;
    td_size = min(nremaining_trb, 31);
    ch = has_more;
    dir = is_dir_in;
  }
 public:
  uint32_t data_buffer_lo;
  uint32_t data_buffer_hi;

  uint32_t trb_transfer_length : 17;
  uint32_t td_size : 5;  // the number of data trbs yet to be sent. Cap to 31
  uint32_t interrupter_target : 10;

  uint32_t c : 1;
  uint32_t ent : 1; // evaluate next trb
  uint32_t isp : 1; // interrupt-on short packet
  uint32_t ns : 1; // no snoop
  uint32_t ch : 1; // chain bit
  uint32_t ioc : 1; // interrupt on completion
  // immediate data. Set to 1 means the data_buffer
  // field contains inlined data (between 1 to 8 bytes)
  // rather than a pointer.
  uint32_t idt : 1;
  uint32_t rsvd : 3;
  uint32_t trb_type : 6;
  // data transfer direction.
  // 0 for out, 1 for in
  uint32_t dir : 1;
  uint32_t rsvd2 : 15;
};

static_assert(sizeof(DataStageTRB) == 16);

class StatusStageTRB : public TRBCommon {
 public:
  explicit StatusStageTRB(bool is_dir_in) : trb_type(STATUS_STAGE) {
    ioc = 1;
    dir = is_dir_in;
  }
 public:
  uint32_t rsvd[2];
  uint32_t rsvd2 : 22;
  uint32_t interrupter_target : 10;

  uint32_t c : 1;
  uint32_t ent : 1;
  uint32_t rsvd3 : 2;
  uint32_t ch : 1;
  uint32_t ioc : 1;
  uint32_t rsvd4 : 4;
  uint32_t trb_type : 6;
  uint32_t dir : 1;
  uint32_t rsvd5 : 15;
};

static_assert(sizeof(StatusStageTRB) == 16);

/*
 * Used by TransferEventTRB and CommandCompletionEventTRB
 */
enum TRBCompletionCode {
  SUCCESS = 1,
  NO_SLOTS_AVAILABLE_ERROR = 9,
  EVENT_RING_FULL_ERROR = 21,
};

static const char* completion_code_to_str(int code) {
  switch (code) {
  case SUCCESS:
    return "success";
  case EVENT_RING_FULL_ERROR:
    return "event_ring_full_error";
  case NO_SLOTS_AVAILABLE_ERROR:
    return "no_slots_available_error";
  default:
    return "unknown";
  }
}

class TransferEventTRB : public TRBCommon {
 public:
  static TRBType TRB_TYPE() {
    return TRANSFER_EVENT;
  }

  // TODO: avoid dupliate the code with CommandCompletionEventTRB::assert_trb_addr
  void assert_trb_addr(TRBTemplate* expect) {
    assert(trb_pointer_high == 0);
    assert(trb_pointer_low == (uint32_t) expect);
  }

  bool has_residue() const {
    return trb_transfer_length > 0;
  }

  // TODO: avoid dupliate the code with CommandCompletionEventTRB
  bool success() {
    if (completion_code != (int) TRBCompletionCode::SUCCESS) {
      printf("Command fail with code %d (%s)\n", completion_code, completion_code_to_str(completion_code));
    }
    return completion_code == (int) TRBCompletionCode::SUCCESS; 
  }
 public:
  uint32_t trb_pointer_low;
  uint32_t trb_pointer_high;

  uint32_t trb_transfer_length : 24;
  uint32_t completion_code : 8;

  uint32_t c : 1;
  uint32_t rsvd : 1;
  uint32_t event_data : 1;
  uint32_t rsvd2 : 7;
  uint32_t trb_type : 6;
  uint32_t endpoint_id : 5;
  uint32_t rsvd3 : 3;
  uint32_t slot_id : 8;
};

static_assert(sizeof(TransferEventTRB) == 16);

class CommandCompletionEventTRB : public TRBCommon {
 public:
  static TRBType TRB_TYPE() {
    return COMMAND_COMPLETION_EVENT;
  }

  // TODO: avoid dupliate the code with TransferEventTRB::assert_trb_addr
  void assert_trb_addr(TRBTemplate* expect) {
    assert(command_trb_pointer_high == 0);
    assert(command_trb_pointer_low == (uint32_t) expect);
  }

  bool success() {
    if (completion_code != (int) TRBCompletionCode::SUCCESS) {
      printf("Command fail with code %d (%s)\n", completion_code, completion_code_to_str(completion_code));
    }
    return completion_code == (int) TRBCompletionCode::SUCCESS; 
  }
 public:
  uint32_t command_trb_pointer_low;
  uint32_t command_trb_pointer_high;
  uint32_t command_completion_parameter : 24;
  uint32_t completion_code : 8;
  uint32_t c : 1;
  uint32_t rsvd : 9;
  uint32_t trb_type : 6;
  uint32_t vf_id : 8;
  uint32_t slot_id : 8;
};

static_assert(sizeof(CommandCompletionEventTRB) == 16);

class HostControllerEventTRB : public TRBCommon {
 public:
  static TRBType TRB_TYPE() {
    return HOST_CONTROLLER_EVENT;
  }
 public:
  uint64_t rsvd;
  uint32_t rsvd1 : 24;
  uint32_t completion_code : 8;
  uint32_t c : 1;
  uint32_t rsvd2 : 9;
  uint32_t trb_type : 6;
  uint32_t rsvd3 : 16;
};

static_assert(sizeof(HostControllerEventTRB) == 16);
