#include <kernel/usb/xhci.h>
#include <kernel/usb/xhci_ring.h>
#include <kernel/usb/usb_device.h>
#include <kernel/usb/msd.h>

TRBTemplate TRBCommon::toTemplate() const {
  return *(TRBTemplate*) this;
}

ProducerTRBRing command_ring;

// TODO: support multiple event ring
ConsumerTRBRing event_ring;

#define AVAIL_TRANSFER_RINGS 128
ProducerTRBRing transfer_ring_pool[AVAIL_TRANSFER_RINGS];
ProducerTRBRing& allocate_transfer_ring() {
  static int next_avail = 0;
  assert(next_avail < AVAIL_TRANSFER_RINGS);
  return transfer_ring_pool[next_avail++];
}

// initialize the command ring. Setup CRCR to point to it.
void XHCIDriver::initializeCommandRing() {
  // set RCS to 1 so the C bit of all TRBS on the command ring
  // can be intialized to 0
  CRCRegister crcc(command_ring.get_addr(), 1 /* rcs */);
  setCRCRLow(crcc.getLow32());
  setCRCRHigh(crcc.getHigh32());

  assert(getCRCRHigh() == 0);

  // getCRCRLow() may return 0 for my toshiba laptop. Maybe the hardware is too
  // old and does not follow the spec.
  #if 0
  // the equality holds since the lowest 6 bits of CRCR are all 0
  assert((getCRCRLow() & ~63)  == (uint32_t) &command_ring);
  #endif
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

// TODO: can we control the lifetime of ths datastructure and save some memory?
InputContext globalInputContextList[256];

void XHCIDriver::initializeContext() {
  constexpr int nCtx = sizeof(globalDeviceContextList) / sizeof(*globalDeviceContextList);
  assert(getMaxSlots() <= nCtx);
  
  // refer to xhci spec '4.20 Scratchpad Buffers' for details.
  int numScratchpad = getMaxScratchpadBuffers();
  if (numScratchpad > 0) {
    assert(numScratchpad * 8 <= 4096);
    globalDeviceContextBaseAddrArray[0] = alloc_phys_page();
    uint64_t* ptr = (uint64_t*) globalDeviceContextBaseAddrArray[0];
    memset(ptr, 0, 4096);
    for (int i = 0; i < numScratchpad; ++i) {
      uint32_t addr = alloc_phys_page();
      memset((void*) addr, 0, 4096);
      ptr[i] = addr;
    }
  } else {
    globalDeviceContextBaseAddrArray[0] = 0;
  }

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

// TODO consolidate with xhch_driver in usb.cpp
XHCIDriver* gcontroller = nullptr;

void update_hc_event_ring_dequeue_ptr(TRBTemplate *dequeue_ptr) {
  assert(gcontroller);
  gcontroller->setERDPLow(0, (uint32_t) dequeue_ptr);
  gcontroller->setERDPHigh(0, 0);
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

  update_hc_event_ring_dequeue_ptr(event_ring.begin());
}

void XHCIDriver::initialize() {
  gcontroller = this;
  initializeCommandRing();
  initializeContext();
  initializeInterrupter();
}

void XHCIDriver::resetPort(int port_no) {
  printf("before reset, port %d, sc 0x%x\n", port_no, getPortSC(port_no));
  setPortSCFlags(port_no, 1 << 4); // port reset
  // the bit will be cleared by xHC
  while (getPortSC(port_no) & (1 << 4)) {
    msleep(1);
  }
  // port reset change bit should be 1 after reset is done
  assert(getPortSC(port_no) & (1 << 21));

  // doing the following on my toshiba laptop cause the current 
  // connect status bit (bit 0) to be reset to 0
  #if 0
  setPortSC(port_no, 1 << 21); // write clear the PRC bit
  assert((getPortSC(port_no) & (1 << 21)) == 0);
  #endif

  printf("After reset, port %d, sc 0x%x\n", port_no, getPortSC(port_no));
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

  command_ring.update_shadow_dequeue_ptr(req);
  return slot_id;
}

void XHCIDriver::initEndpoint0Context(InputContext& input_context, int max_packet_size) {
  ProducerTRBRing& transfer_ring = allocate_transfer_ring();
  EndpointContext& ep0_context = input_context.device_context().endpoint_context(0);
  ep0_context.ep_type = EndpointType::CONTROL;
  ep0_context.max_packet_size = max_packet_size;
  printf("ep0 max packet size is %d\n", max_packet_size);
  ep0_context.max_burst_size = 0;
  ep0_context.set_tr_dequeue_pointer(transfer_ring.get_addr());
  ep0_context.dcs = 1;
  ep0_context.interval = 0;
  ep0_context.max_pstreams = 0;
  ep0_context.mult = 0;
  ep0_context.cerr = 3;
}

void XHCIDriver::initEndpointContext(InputContext& input_context, EndpointDescriptor& endpoint_descriptor) {
  ProducerTRBRing& transfer_ring = allocate_transfer_ring();
  bool is_in = (endpoint_descriptor.bEndpointAddress & 0x80);
  EndpointContext& ep_context = input_context.device_context().endpoint_context(
    endpoint_descriptor.bEndpointAddress & 0xF,
    is_in
  );
  // TODO: only support bulk endpoint for now
  ep_context.ep_type = is_in ? EndpointType::BULK_IN : EndpointType::BULK_OUT;
  ep_context.max_packet_size = endpoint_descriptor.wMaxPacketSize;
  ep_context.max_burst_size = 0;  // TODO use the information in SuperSpeedEndpointCompanionDescriptor to initialize it?
  ep_context.set_tr_dequeue_pointer(transfer_ring.get_addr());
  ep_context.dcs = 1;
  ep_context.interval = 0;
  ep_context.max_pstreams = 0;
  ep_context.mult = 0;
  ep_context.cerr = 3;
}

void XHCIDriver::setup_slot(uint32_t slot_id, int port_no, int max_packet_size) {
  InputContext& input_context = globalInputContextList[slot_id];
  DeviceContext& output_context = globalDeviceContextList[slot_id];
  input_context.control_context().include_add_context_flags(0x3);

  // xhci spec 4.3.3: device slot initialization
  {
    // initialize input slot context
    SlotContext& input_slot_context = input_context.device_context().slot_context();
    input_slot_context.root_hub_port_number = port_no;
    input_slot_context.route_string = 0;
    // XHCI spec 4.5.2 mentions context_entries should be set to 1 for
    // Address Device Command.
    input_slot_context.context_entries = 1;
  }
  initEndpoint0Context(input_context, max_packet_size);
}

uint32_t XHCIDriver::assign_usb_device_address(uint32_t slot_id) {
  InputContext& input_context = globalInputContextList[slot_id];
  DeviceContext& output_context = globalDeviceContextList[slot_id];
  // xhci spec 4.3.4: address assignment
  sendCommand(AddressDeviceCommandTRB((uint32_t) &input_context, slot_id).toTemplate());

  uint32_t usb_device_address = output_context.slot_context().usb_device_address;
  assert(&output_context.endpoint_context(0).get_tr_dequeue_pointer() == &input_context.device_context().endpoint_context(0).get_tr_dequeue_pointer());
  return usb_device_address;
}

void XHCIDriver::sendCommand(TRBTemplate req) {
  TRBTemplate* req_addr = command_ring.enqueue(req);
  ringDoorbell(0, 0); 
  TRBTemplate resp = event_ring.dequeue();
  CommandCompletionEventTRB command_comp = resp.expect_trb_type<CommandCompletionEventTRB>();
  command_comp.assert_trb_addr(req_addr);
  assert(command_comp.success());
  command_ring.update_shadow_dequeue_ptr(req_addr);
}

static uint32_t add_context_flags_for_configure_endpoint(EndpointDescriptor& bulk_in, EndpointDescriptor bulk_out) {
  uint32_t flags = 0;
  flags |= 1; // pick the slot context
  flags |= (1 << DeviceContext::endpoint_context_index(bulk_in.bEndpointAddress & 0xF, true));
  assert(bulk_in.bEndpointAddress & 0x80);
  flags |= (1 << DeviceContext::endpoint_context_index(bulk_out.bEndpointAddress & 0xF, false));
  assert((bulk_out.bEndpointAddress & 0x80) == 0);
  return flags;
}

// send configure endpoint command to the command ring.
void XHCIDriver::configureEndpoints(USBDevice<XHCIDriver>* dev) {
  uint32_t slot_id = dev->slot_id();
  InputContext& input_context = globalInputContextList[slot_id];

  SlotContext& input_slot_context = input_context.device_context().slot_context();

  // follow Haiku OS
  input_slot_context.context_entries = 31;

  initEndpointContext(input_context, dev->get_bulk_in_endpoint_desc());
  initEndpointContext(input_context, dev->get_bulk_out_endpoint_desc());

  InputControlContext& control_context = input_context.control_context();
  control_context.drop_context_flags() = 0;
  control_context.add_context_flags() = add_context_flags_for_configure_endpoint(dev->get_bulk_in_endpoint_desc(), dev->get_bulk_out_endpoint_desc());

  // send the configure endpoint command
  TRBTemplate req = ConfigureEndpointCommandTRB((uint32_t) &input_context, slot_id).toTemplate();
  sendCommand(req);
}

void XHCIDriver::initializeDevice(USBDevice<XHCIDriver> *dev) {
  uint32_t slot_id = allocate_device_slot();
  printf("Got device slot %d\n", slot_id);

  int port_no = getSolePortNo();
  dev->setMaxPacketSizeByPortSpeed(getPortSpeed(port_no));
  setup_slot(slot_id, port_no, dev->getMaxPacketSize());

  uint32_t usb_device_address = assign_usb_device_address(slot_id);
  printf("Assigned usb device address %d\n", usb_device_address);

  // xhci spec: 4.3.5 device configuration
  dev->slot_id() = slot_id;

  DeviceDescriptor device_desc = dev->getDeviceDescriptor();
  printf("Device USB version 0x%x\n", device_desc.bcdUSB);
  ConfigurationDescriptor config_desc = dev->getConfigurationDescriptor();

  // set configuration
  dev->setConfiguration(config_desc.bConfigurationValue);
  assert(dev->getConfiguration() == config_desc.bConfigurationValue);
  printf("The device is configured with config value %d\n", config_desc.bConfigurationValue);

  dev->collectEndpointDescriptors();

  // configure endpoints
  configureEndpoints(dev);

  printf("Current slot state: %s\n", globalDeviceContextList[slot_id].slot_context().get_state_str());
}

void XHCIDriver::sendDeviceRequest(USBDevice<XHCIDriver>* device, DeviceRequest* device_req, void *buf) {
  assert(device->slot_id() >= 0);

  EndpointContext& input_ep0_context = globalInputContextList[device->slot_id()].device_context().endpoint_context(0);
  // NOTE: the tr_dequeue_pointer read from the input context can represent the address of the transfer ring.
  // (can not use the output device context here since the tr_dequeue_pointer for the output device context will
  // be updated by the controller)
  ProducerTRBRing& transfer_ring = input_ep0_context.get_tr_dequeue_pointer();

  transfer_ring.enqueue(SetupStageTRB(device_req).toTemplate());

  int maxPacketSize = device->getMaxPacketSize();
  int ndata_trb = (device_req->wLength + maxPacketSize - 1) / maxPacketSize;

  uint32_t off = 0, len;
  for (int i = 0; i < ndata_trb; ++i) {
    len = min(maxPacketSize, device_req->wLength - off);
    assert(len > 0);
    auto data_trb = DataStageTRB((uint32_t) buf + off, len, i != ndata_trb - 1, ndata_trb - i - 1, !!(device_req->bmRequestType & 0x80));
    transfer_ring.enqueue(data_trb.toTemplate());
    off += len;
  }

  // Refert to XHCI spec table 4-7 for how to define DIR bit of StatusStageTRB
  auto status_trb = StatusStageTRB(!(device_req->bmRequestType & 0x80) || device_req->wLength == 0);
  auto* status_trb_addr = transfer_ring.enqueue(status_trb.toTemplate());
  ringDoorbell(device->slot_id(), 1 /* for control endpoint 0 */);

  TRBTemplate event_trb_template = event_ring.dequeue();
  TransferEventTRB event_trb = event_trb_template.expect_trb_type<TransferEventTRB>();
  assert(event_trb.success());
  event_trb.assert_trb_addr(status_trb_addr);
  assert(!event_trb.has_residue());
  assert(event_trb.endpoint_id == 1);
  assert(event_trb.slot_id == device->slot_id());

  transfer_ring.update_shadow_dequeue_ptr(status_trb_addr);
}

static int get_endpoint_context_idx(const EndpointDescriptor& desc) {
  int ep_num = (desc.bEndpointAddress & 0xF);
  bool is_in = (desc.bEndpointAddress & 0x80);
  return DeviceContext::endpoint_context_index(ep_num, is_in);
}

static EndpointContext& get_endpoint_context(USBDevice<XHCIDriver>* device, const EndpointDescriptor& desc, bool input_context) {
  DeviceContext* device_context_ptr = nullptr;
  if (input_context) {
    device_context_ptr = &globalInputContextList[device->slot_id()].device_context();
  } else {
    device_context_ptr = &globalDeviceContextList[device->slot_id()];
  }
  DeviceContext& device_context = *device_context_ptr;
  int ep_num = (desc.bEndpointAddress & 0xF);
  bool is_in = (desc.bEndpointAddress & 0x80);
  EndpointContext& endpoint_context = device_context.endpoint_context(ep_num, is_in);
  return endpoint_context;
}

// common code for bulkSend/bulkRecv
void XHCIDriver::bulkTransfer(USBDevice<XHCIDriver>* device, const EndpointDescriptor& desc, const uint8_t* buf, int bufsize) {
  EndpointContext& input_ctx = get_endpoint_context(device, desc, true);

  int maxPacketSize = desc.wMaxPacketSize;
  int ntd = (bufsize + maxPacketSize - 1) / maxPacketSize;
  assert(bufsize > 0);

  ProducerTRBRing &transfer_ring = input_ctx.get_tr_dequeue_pointer();

  int off = 0, len;
  TRBTemplate* expected_trb = nullptr;
  for (int i = 0; i < ntd; ++i) {
    len = min(maxPacketSize, bufsize - off);
    NormalTRB normal_trb((uint32_t) buf + off, len, i == ntd - 1);
    expected_trb = transfer_ring.enqueue(normal_trb.toTemplate()); // only the last assignment matters
    off += len;
  }

  ringDoorbell(device->slot_id(), get_endpoint_context_idx(desc));

  TRBTemplate event_trb_template = event_ring.dequeue();
  if (!event_trb_template.to_trb_type<TransferEventTRB>()) {
    HostControllerEventTRB* hce_trb = event_trb_template.to_trb_type<HostControllerEventTRB>();
    if (hce_trb) {
      printf("HostControllerEventTRB completion code %d\n", hce_trb->completion_code);
    }
    assert(false && "bulkTransfer fail to get a transfer event trb\n");
  }
  TransferEventTRB event_trb = event_trb_template.expect_trb_type<TransferEventTRB>();
  event_trb.assert_trb_addr(expected_trb);
  transfer_ring.update_shadow_dequeue_ptr(expected_trb);
  assert(!event_trb.has_residue());
  assert(event_trb.success());
  assert(event_trb.endpoint_id == get_endpoint_context_idx(desc));
  assert(event_trb.slot_id == device->slot_id());
}

void XHCIDriver::bulkSend(USBDevice<XHCIDriver>* device, const EndpointDescriptor& desc, uint32_t& /* toggle */, const uint8_t* buf, int bufsize) {
  bulkTransfer(device, desc, buf, bufsize);
}

void XHCIDriver::bulkRecv(USBDevice<XHCIDriver>* device, const EndpointDescriptor& desc, uint32_t& /* toggle */, uint8_t* buf, int bufsize) {
  bulkTransfer(device, desc, buf, bufsize);
}
