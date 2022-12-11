#pragma once

#include <assert.h>
#include <stdint.h>

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
  SUPERSPEED_USB_ENDPOINT_COMPANION = 48,
};

/*
 * Per xhci spec 9.6.7 an endpoint companion descriptor (if exist) should immediately
 * follow an endpoint descriptor it is associated with in the configuration
 * information.
 * TODO: We just ignore this descriptor for now.
 */
class SuperSpeedEndpointCompanionDescriptor {
 public:
  uint8_t dummy[6];
};

static_assert(sizeof(SuperSpeedEndpointCompanionDescriptor) == 6);

class EndpointDescriptor {
 public:
  EndpointDescriptor() : bLength(0) {
  }

  operator bool() const {
    return bLength != 0;
  }
  void print() const {
    printf("ENDPOINT DESC:\n"
      "  addr 0x%x, attr 0x%x, max packet size %d\n",
      bEndpointAddress, bmAttributes, wMaxPacketSize);
    ensure();
  }
  void ensure() const {
    assert(bDescriptorType == (uint8_t) DescriptorType::ENDPOINT);
    assert(bLength == sizeof(EndpointDescriptor));
  }

  uint8_t bLength;
  uint8_t bDescriptorType;
  // bit 3..0 the endpoint nubmer
  // bit 6..4, reserved, reset to 0
  // bit 7: direction, ignore for control endpoints.
  //        0 = out endpoint
  //        1 = in endpoint
  uint8_t bEndpointAddress;
  // bit 1..0: transfer type
  // 00 = control
  // 01 = isochronous
  // 10 = bulk
  // 11 = interrupt.
  // Check usb3 spec 9.6.6 for explanation of other bits.
  uint8_t bmAttributes;
  uint16_t wMaxPacketSize;
  uint8_t bInterval;
} __attribute__((packed));

static_assert(sizeof(EndpointDescriptor) == 7);

class InterfaceDescriptor {
 public:
  void print() const {
    ensure();
    printf("INTERFACE DESC:\n"
      "  interface no %d, alter setting %d, num endpoints %d\n"
      "  class 0x%x, subclss 0x%x, protocol 0x%x\n"
      "  iInterface %d\n",
      bInterfaceNumber, bAlternateSetting, bNumEndpoints,
      bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol,
      iInterface);
  }
  void ensure() const {
    assert(bLength == sizeof(InterfaceDescriptor));
    assert(bDescriptorType == (uint8_t) DescriptorType::INTERFACE);
  }
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bInterfaceNumber;
  uint8_t bAlternateSetting;
  // number of endpoints used by the interface (excluding the default control pipe).
  uint8_t bNumEndpoints;
  uint8_t bInterfaceClass;
  uint8_t bInterfaceSubClass;
  uint8_t bInterfaceProtocol;
  // index of string descriptor describing this interface
  uint8_t iInterface;
};

static_assert(sizeof(InterfaceDescriptor) == 9);

class ConfigurationDescriptor {
 public:
  void print() const {
    printf("CONFIG DESC:\n"
      "  tot len %d, num interface %d, config val %d\n",
      wTotalLength, bNumInterfaces, bConfigurationValue);
  }

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
