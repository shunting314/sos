#pragma once

#include <assert.h>
#include <stdint.h>

class ConfigurationDescriptor {
 public:
  uint8_t bLength;
  uint8_t bDescriptorType;
  // total length of data returned for this descriptor and the associated
  // interface, enterpointer descriptors etc.
  uint16_t wTotalLength;
  uint8_t bNumInterfaces;
  uint8_t bConfigurationValue;
  // index of string descriptor describing this configuration
  uint8_t iConfiguration;
  uint8_t bmAttributes;
  uint8_t bMaxPower;
} __attribute__((packed));

static_assert(sizeof(ConfigurationDescriptor) == 9);

class DeviceDescriptor {
 public:
  uint8_t bLength;
  uint8_t bDescriptorType; // should be Device Descriptor Type
  uint16_t bcdUSB; // USB spec release number in BCD form
  uint8_t bDeviceClass;
  uint8_t bDeviceSubClass;
  uint8_t bDeviceProtocol;
  // USB3 spec says the max packet size should be 2^bMaxPacketSize0;
  // However the usb osdev wiki says this field specifies the max packet size
  // directly.
  //
  // For a MSD QEMU simulated, this field returns 8. I can read a 8 bytes packet but
  // not 18 bytes packet for the DeviceDescriptor. So I think for this device
  // it conforms to what osdev wiki says.
  uint8_t bMaxPacketSize0; // max packet size for endpoint 0
  uint16_t idVendor;
  uint16_t idProduct;
  uint16_t bcdDevice; // device release number in BCD form
  uint8_t iManufacturer; // index of STRING descriptor describing manufacturer
  uint8_t iProduct;
  uint8_t iSerialNumber;
  uint8_t bNumConfigurations;
};

static_assert(sizeof(DeviceDescriptor) == 18);

// for the GET_DESCRIPTOR device request
// copied from the osdev wiki for usb
enum class DescriptorType : uint8_t {
  DEVICE = 1,
  CONFIGURATION = 2,
  STRING = 3,
  INTERFACE = 4,
  ENDPOINT = 5,
  DEVICE_QUALIFIER = 6,
  OTHER_SPEED_CONFIGURATION = 7,
  INTERFACE_POWER = 8,
};

enum class DeviceRequestCode : uint8_t {
  SET_ADDRESS = 5,
  GET_DESCRIPTOR = 6,
  GET_CONFIGURATION = 8,
  SET_CONFIGURATION = 9,
};

// a device request is the payload of the Setup packet.
class DeviceRequest {
 public:
  explicit DeviceRequest(
    uint8_t _bmRequestType,
    uint8_t _bRequest,
    uint16_t _wValue,
    uint16_t _wIndex,
    uint16_t _wLength
  ) : bmRequestType(_bmRequestType),
      bRequest(_bRequest),
      wValue(_wValue),
      wIndex(_wIndex),
      wLength(_wLength) {
  }

  void print() const {
    printf("DeviceRequest: type 0x%x, req %d, value 0x%x, idx 0x%x, len %d\n", bmRequestType, bRequest, wValue, wIndex, wLength);
  }

  uint8_t bmRequestType;
  uint8_t bRequest; // specific request type. Check DeviceRequestCode
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t wLength;
};
static_assert(sizeof(DeviceRequest) == 8);
