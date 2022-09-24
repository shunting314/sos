#pragma once

class DeviceDescriptor {
 public:
  uint8_t bLength;
  uint8_t bDescriptorType; // should be Device Descriptor Type
  uint16_t bcdUSB; // USB spec release number in BCD form
  uint8_t bDeviceClass;
  uint8_t bDeviceSubClass;
  uint8_t bDeviceProtocol;
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
  GET_DESCRIPTOR = 6,
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
  uint8_t bmRequestType;
  uint8_t bRequest; // specific request type. Check DeviceRequestCode
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t wLength;
};
static_assert(sizeof(DeviceRequest) == 8);
