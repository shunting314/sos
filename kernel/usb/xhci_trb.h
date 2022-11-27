#pragma once

enum TRBType {
  LINK = 6,
  NO_OP = 8,
  ENABLE_SLOT_COMMAND = 9,
  ADDRESS_DEVICE_COMMAND = 11,
  COMMAND_COMPLETION_EVENT = 33,
  PORT_STATUS_CHANGE_EVENT = 34,
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
  case COMMAND_COMPLETION_EVENT:
    return "command_completion_event";
  case PORT_STATUS_CHANGE_EVENT:
    return "port_status_change_event";
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
    assert(trb_type == T::TRB_TYPE());
    return *(T*) this;
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

class LinkTRB : public TRBCommon {
 public:
  explicit LinkTRB(uint32_t segment_addr) {
    assert((segment_addr & 0xF) == 0);  // 16 byte align
    ring_segment_pointer_low = segment_addr;
    tc = 1; // set toggle bit
    trb_type = TRBType::LINK;
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

enum TRBCompletionCode {
  SUCCESS = 1,
};

class CommandCompletionEventTRB : public TRBCommon {
 public:
  static TRBType TRB_TYPE() {
    return COMMAND_COMPLETION_EVENT;
  }

  void assert_trb_addr(TRBTemplate* expect) {
    assert(command_trb_pointer_high == 0);
    assert(command_trb_pointer_low == (uint32_t) expect);
  }

  bool success() {
    if (completion_code != (int) TRBCompletionCode::SUCCESS) {
      printf("Command fail with code %d\n", completion_code);
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
