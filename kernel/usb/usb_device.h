#pragma once

#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/usb/usb_proto.h>
#include <kernel/scsi.h>

// TODO: instead of using template, use a controller driver base class
template <typename ControllerDriver>
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
      visitInterfaceDesc(ifdesc_ptr);

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

        // an endpoint descriptor may be followed by a SuperSpeedEndpointCompanionDescriptor.
        // TODO: should we not simply ignore SuperSpeedEndpointCompanionDescriptor?
        if (off + 2 <= config_desc.wTotalLength) {
          uint8_t nextlen = *(buf + off);
          uint8_t nexttype = *(buf + off + 1);
          if (nexttype == (int) DescriptorType::SUPERSPEED_USB_ENDPOINT_COMPANION) {
            assert(nextlen == sizeof(SuperSpeedEndpointCompanionDescriptor));
            assert(off + nextlen <= config_desc.wTotalLength);
            off += nextlen;
          }
        }
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

  /*
   * xhci spec '4.3 USB Device Initialization' item 7 mentions, for LS, HS, and SS
   * devices, 8, 64 and 512 bytes, respectively, are the only packet sizes allowed
   * for the Default Control Endpoint.
   *
   * For FS device, I'll set max packet size to 8 as Haiku os does.
   * We can update it with the information we get from DeviceDescriptor later
   * if we want.
   */
  void setMaxPacketSizeByPortSpeed(int portSpeed) {
    switch (portSpeed) {
    case 1: // FS
    case 2: // LS
      max_packet_size_ = 8;
      break;
    case 3: // HS
      max_packet_size_ = 64;
      break;
    case 4: // SS
      max_packet_size_ = 512;
      break;
    default:
      printf("Unsupported port speed %d\n", portSpeed);
      assert(false && "Unsupported port speed");
    }
  }

  // max packet length for endpoint 0
  uint16_t getMaxPacketSize() {
    assert(max_packet_size_ > 0);
    return max_packet_size_;
  }

  uint8_t getAddr() const { return addr_; }

  uint32_t& slot_id() {
    return slot_id_;
  }

  EndpointDescriptor& get_bulk_out_endpoint_desc() { return bulkOut_; }
  EndpointDescriptor& get_bulk_in_endpoint_desc() { return bulkIn_; }
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

  void visitInterfaceDesc(InterfaceDescriptor* descPtr) {
    // TODO: we only support MSD so far
    assert(descPtr->bInterfaceClass == 0x08); // MASS STORAGE class
    // 0x06 is the subclass code USB-IF assigns to SCSI. The definition of
    // SCSI command sets is out of the scope of USB spec. Need refer to SCSI
    // spec to understand how to setup the command in a CommandBlockWrapper
    assert(descPtr->bInterfaceSubClass == 0x06);
    assert(descPtr->bInterfaceProtocol == 0x50); // BULK-ONLY Transport
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
 protected:
  ControllerDriver* controller_driver_;
  uint8_t addr_; // device address

  // TODO: There are specific to MSD. Should move them to a MSD specific class
  uint32_t bulkOutDataToggle_ = 0;
  EndpointDescriptor bulkOut_;
  uint32_t bulkInDataToggle_ = 0;
  EndpointDescriptor bulkIn_;

  // only needed for XHCI
  uint32_t slot_id_ = -1;
  int max_packet_size_ = -1; // max packet size for endpoint 0
};
