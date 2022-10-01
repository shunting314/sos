#pragma once

#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <kernel/usb/usb_proto.h>

// TODO: use some controller driver base class rather than UHCIDriver
#define ControllerDriver UHCIDriver

class USBDevice {
 public:
  explicit USBDevice(ControllerDriver* driver)
    : controller_driver_(driver), addr_(0) {
  }

  void assignAddress() {
    assert(addr_ == 0);
    uint8_t addr = controller_driver_->acquireAvailAddr();
    assert(addr > 0 && addr <= 127);
    DeviceRequest req = createSetAddressRequest(addr);
    controller_driver_->sendDeviceRequest(this, &req, nullptr);

    // NOTE: it's very important to set addr_ after the sendDeviceRequest call.
    // Inside sendDeviceRequest, the controller driver will use addr_ as the
    // address to talk to the device. But for the SET_ADDRESS request itself,
    // we must use the default address (NOT the one to be assigne) since the
    // address has not be assigned to the device yet.
    addr_ = addr;
  }

  DeviceDescriptor getDeviceDescriptor() {
    DeviceRequest req = createGetDeviceDescriptorRequest();
    DeviceDescriptor desc;
    controller_driver_->sendDeviceRequest(this, &req, &desc);
    return desc;
  }

  // short hands to create DeviceRequest
  DeviceRequest createGetDeviceDescriptorRequest() {
    return DeviceRequest(
      0x80 /* bmRequestType */,
      (uint8_t) DeviceRequestCode::GET_DESCRIPTOR,
      (uint16_t) DescriptorType::DEVICE << 8,
      0,
      sizeof(DeviceDescriptor)
    );
  }

  DeviceRequest createSetAddressRequest(uint8_t addr) {
    return DeviceRequest(
      0x0,
      (uint8_t) DeviceRequestCode::SET_ADDRESS,
      addr,
      0,
      0
    );
  }

  uint16_t getMaxPacketLength() {
    // TODO: 8 is quite conservative. Can we use a larger value here?
    return 8;
  }

  uint8_t getAddr() const { return addr_; }
 private:
  ControllerDriver* controller_driver_;
  uint8_t addr_; // device address
};
