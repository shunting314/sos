#include <kernel/usb/xhci.h>
#include <kernel/usb/xhci_ring.h>

TRBTemplate TRBCommon::toTemplate() const {
  return *(TRBTemplate*) this;
}

ProducerTRBRing command_ring(true);

// TODO: support multiple event ring
ConsumerTRBRing event_ring;

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

// a single event ring segment.
EventRingSegmentTableEntry event_ring_segment_table[1] __attribute__((aligned(64)));

EventRingSegmentTableEntry::EventRingSegmentTableEntry(const TRBRing& trb_ring) {
  memset(this, 0, sizeof(*this));
  ring_segment_base_address_low = trb_ring.get_addr();
  assert((ring_segment_base_address_low & 63) == 0);
  ring_segment_base_address_high = 0;
  ring_segment_size = trb_ring.trb_capacity();
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

  // a tricky part here is, event_ring_segment_table must be setup before its
  // address is provided to the controller. The controller may read the table
  // when the address is given. Setting up the table after that is too late.
  event_ring_segment_table[0] = EventRingSegmentTableEntry(event_ring);
  setERSTBALow(0, (uint32_t) &event_ring_segment_table);
  setERSTBAHigh(0, 0);

  setERDPLow(0, event_ring.get_addr());
  setERDPHigh(0, 0);
}

void XHCIDriver::initialize() {
  initializeCommandRing();
  initializeContext();
  initializeInterrupter();
}

void XHCIDriver::resetPort(int port_no) {
  setPortSCFlags(port_no, 1 << 4); // port reset
  // the bit will be cleared by xHC
  while (getPortSC(port_no) & (1 << 4)) {
    msleep(1);
  }
  // port reset change bit should be 1 after reset is done
  assert(getPortSC(port_no) & (1 << 21));
  setPortSC(port_no, 1 << 21); // write clear the PRC bit
  assert((getPortSC(port_no) & (1 << 21)) == 0);
}

uint32_t XHCIDriver::allocate_device_slot() {
  event_ring.skip_queued_trbs();
  TRBTemplate* req = command_ring.enqueue(EnableSlotCommandTRB().toTemplate());
  ringDoorbell(0, 0);
  TRBTemplate resp = event_ring.dequeue();

  CommandCompletionEventTRB command_comp = resp.expect_trb_type<CommandCompletionEventTRB>();
  command_comp.assert_trb_addr(req);
  assert(command_comp.success());
  int slot_id = command_comp.slot_id;
  // According to xhci spec 4.6.3, slot id 0 represents no slots available.
  assert(slot_id > 0);
  printf("Got slot %d\n", slot_id);

  // TODO: update event ring dequeue pointer
  return slot_id;
}

void XHCIDriver::initializeDevice() {
  printf("Got device slot %d\n", allocate_device_slot());
  assert(false && "init device");
}
