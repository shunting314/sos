#include <kernel/usb/xhci.h>

TRBTemplate TRBCommon::toTemplate() const {
  return *(TRBTemplate*) this;
}

// All TRBBase on the list contains all 0 initially. This is archieved by the
// TRBCommon ctor.
class TRBRing {
 public:
  explicit TRBRing() {
    assert((get_addr() & 0xFFF) == 0); // page alignment
    assert(sizeof(trb_ring_) == 4096);

    trb_ring_[capacity() - 1] = LinkTRB(get_addr()).toTemplate();
  }

  uint32_t capacity() const {
    return sizeof(trb_ring_) / sizeof(*trb_ring_);
  }

  uint32_t get_addr() const {
    return (uint32_t) &trb_ring_;
  }
 private:
  // 256 items
  TRBTemplate trb_ring_[4096 / sizeof(TRBTemplate)] __attribute__((aligned(4096))); 
};

TRBRing command_ring;

// initialize the command ring. Setup CRCR to point to it.
void XHCIDriver::initializeCommandRing() {
  // set RCS to 1 so the C bit of all TRBS on the command ring
  // can be intialized to 0
  CRCRegister crcc(command_ring.get_addr(), 1 /* rcs */);
  setCRCRLow(crcc.getLow32());
  setCRCRHigh(crcc.getHigh32());

  assert(getCRCRHigh() == 0);
  // the equality holds since the lowest 6 bits of CRCR are all 0
  assert((getCRCRLow() & ~63)  == (uint32_t) &command_ring);
}

// NOTE: 256 may be more then needed since MaxSlots reported by the xHC
// may be smaller.
// TODO: dynamicly allocate the array with a precise size.
// TODO: an alternative is set the DCBA to 0 for the corresponding DeviceSlot
// so xHC will not use the entry (which can be missing).
// Refer to xHCI spec 6.1 for details.
DeviceContext globalDeviceContextList[256];
// 256 entries are again may be more than needed
// Entry 0 is for scratchpad if enabled or 0 otherwise. Check xHCI spec 6.1 for
// details.
uint64_t globalDeviceContextBaseAddrArray[256] __attribute__((aligned(64)));

void XHCIDriver::initializeContext() {
  constexpr int nCtx = sizeof(globalDeviceContextList) / sizeof(*globalDeviceContextList);
  assert(getMaxSlots() <= nCtx);
  globalDeviceContextBaseAddrArray[0] = 0;
  for (int i = 1; i < nCtx; ++i) {
    globalDeviceContextBaseAddrArray[i] = (uint32_t) &globalDeviceContextList[i];
  }

  setDCBAAPLow((uint32_t) &globalDeviceContextBaseAddrArray);
  setDCBAAPHigh(0);
}

// TODO: support multiple event ring
TRBRing event_ring;

// a single event ring segment.
EventRingSegmentTableEntry event_ring_segment_table[1] __attribute__((aligned(64)));

EventRingSegmentTableEntry::EventRingSegmentTableEntry(const TRBRing& trb_ring) {
  memset(this, 0, sizeof(*this));
  ring_segment_base_address_low = trb_ring.get_addr();
  assert((ring_segment_base_address_low & 63) == 0);
  ring_segment_base_address_high = 0;
  ring_segment_size = trb_ring.capacity();
}

/*
 * According to xhci spec 5.5.2, secondary interrupters can be initialized
 * after R/S is set to 1 but before a event targeting it is generated.
 *
 * SOS only uses the primary interrupter for now.
 */
void XHCIDriver::initializeInterrupter() {
  printf("Interrupter management register 0: 0x%x\n", getIMan(0));
  printf("Interrupter moderation register 0: 0x%x\n", getIMod(0));
  printf("ERSTSZ 0: %d\n", getERSTSZ(0));

  setERSTSZ(0, 1);  // a single event ring segment
  setERSTBALow(0, (uint32_t) &event_ring_segment_table);
  setERSTBAHigh(0, 0);

  setERDPLow(0, event_ring.get_addr());
  setERDPHigh(0, 0);

  event_ring_segment_table[0] = EventRingSegmentTableEntry(event_ring);
}

void XHCIDriver::initialize() {
  initializeCommandRing();
  initializeContext();
  initializeInterrupter();
  assert(false && "initialize ni");
}
