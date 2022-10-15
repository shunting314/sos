#pragma once

#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
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

  ConfigurationDescriptor getConfigurationDescriptor() {
    ConfigurationDescriptor desc;
    DeviceRequest req = createGetConfigurationDescriptorRequest(sizeof(desc));
    controller_driver_->sendDeviceRequest(this, &req, &desc);
    return desc;
  }

  void collectEndpointDescriptors() {
    visitTransitiveConfigurationDescriptor();
    // TODO: assume there is a single bulk in/out endpoint.
    // make this more generic
    assert(bulkOut_);
    assert(bulkIn_);
  }

  void visitTransitiveConfigurationDescriptor() {
    auto config_desc = getConfigurationDescriptor();
    uint8_t buf[256]; // assume 256 bytes is enough
    assert(config_desc.wTotalLength <= sizeof(buf));
    assert(config_desc.wTotalLength >= sizeof(config_desc));
    DeviceRequest req = createGetConfigurationDescriptorRequest(config_desc.wTotalLength);
    controller_driver_->sendDeviceRequest(this, &req, buf);

    assert(memcmp(&config_desc, buf, sizeof(config_desc)) == 0);
    config_desc.print();

    int off = sizeof(config_desc);
    // a configuration has at least one interface. Each interface has zero or more
    // endpoints.
    assert(config_desc.bNumInterfaces > 0);

    // at least 2 more bytes for length and type
    assert(off + 1 < config_desc.wTotalLength);

    // From usb3 spec section 9.6.5, endpoint descriptors follows the corresponding
    // interface descriptor. I.e., the data is ordered following a pre-order DFS
    // traversal.
    for (int if_idx = 0; if_idx < config_desc.bNumInterfaces; ++if_idx) {
      uint8_t len = *(buf + off);
      uint8_t type = *(buf + off + 1);
      assert(off + len <= config_desc.wTotalLength);

      InterfaceDescriptor* ifdesc_ptr = (InterfaceDescriptor*) (buf + off);
      ifdesc_ptr->print(); // ensure the type and length inside print method
      off += len;

      // NOTE: default control pipe does not show up in the endpoint descriptor list.
      for (int ep_idx = 0; ep_idx < ifdesc_ptr->bNumEndpoints; ++ep_idx) {
        assert(off + 1 < config_desc.wTotalLength);
        len = *(buf + off);
        type = *(buf + off + 1);
        assert(off + len <= config_desc.wTotalLength);

        EndpointDescriptor* eddesc_ptr = (EndpointDescriptor*) (buf + off);
        eddesc_ptr->print();
        off += len;
        // TODO: don't support endpoint companion descriptors following endpoint descriptors so far.
        // usb3 spec section 9.6.5 mentions when this will happen.
        visitEndpointDesc(eddesc_ptr);
      }
    }

    assert(off == config_desc.wTotalLength);
  }

  void setConfiguration(uint8_t configVal) {
    DeviceRequest req = createSetConfigurationRequest(configVal);
    controller_driver_->sendDeviceRequest(this, &req, nullptr);
  }

  uint8_t getConfiguration() {
    uint8_t configVal;
    DeviceRequest req = createGetConfigurationRequest();
    controller_driver_->sendDeviceRequest(this, &req, &configVal);
    return configVal;
  }

  uint16_t getMaxPacketLength() {
    // TODO: 8 is quite conservative. Can we use a larger value here?
    return 8;
  }

  uint8_t getAddr() const { return addr_; }
 private:
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

  // the conguration descriptor returned can be followed by interface and endpoint
  // descriptors. So the length can be larger than sizeof(ConfigurationDescriptor)
  DeviceRequest createGetConfigurationDescriptorRequest(uint16_t length) {
    return DeviceRequest(
      0x80,
      (uint8_t) DeviceRequestCode::GET_DESCRIPTOR,
      (uint16_t) DescriptorType::CONFIGURATION << 8,
      0,
      length
    );
  }

  DeviceRequest createSetConfigurationRequest(uint8_t config_val) {
    return DeviceRequest(
      0x0,
      (uint8_t) DeviceRequestCode::SET_CONFIGURATION,
      (uint16_t) config_val,
      0,
      0
    );
  }

  DeviceRequest createGetConfigurationRequest() {
    return DeviceRequest(
      0x80,
      (uint8_t) DeviceRequestCode::GET_CONFIGURATION,
      0,
      0,
      1
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

  void visitEndpointDesc(EndpointDescriptor* descPtr) {
    assert((descPtr->bmAttributes & 3) == 2 && "Only expect bulk endpoint so far");
    if (descPtr->bEndpointAddress & 0x80) {
      // in
      assert(!bulkIn_);
      bulkIn_ = *descPtr;
    } else {
      // out
      assert(!bulkOut_);
      bulkOut_ = *descPtr;
    }
  }

  ControllerDriver* controller_driver_;
  uint8_t addr_; // device address

  // TODO: There are specific to MSD. Should move them to a MSD specific class
  EndpointDescriptor bulkOut_;
  EndpointDescriptor bulkIn_;
};
