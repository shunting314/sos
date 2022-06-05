#pragma once
#include <assert.h>
#include <stdint.h>
#include <kernel/ioport.h>

#define PORT_CONFIG_ADDRESS 0xCF8
#define PORT_CONFIG_DATA 0xCFC

#define MAX_NUM_BUS 256 // max number of pci buses
#define MAX_NUM_DEVICES_PER_BUS 32 // max number of devices per bus
#define MAX_NUM_FUNCTIONS_PER_DEVICE 8 // max number of functions per device

// check PCI osdev wiki
#define CONFIG_OFF_VENDOR_ID 0x0 // 2 byte size
#define CONFIG_OFF_DEVICE_ID 0x2 // 2 byte size
#define CONFIG_OFF_CLASS_CODE 0xB // 1 byte size
#define CONFIG_OFF_SUBCLASS_CODE 0xA // 1 byte size
#define CONFIG_OFF_HEADER_TYPE 0xE // 1 byte size

void lspci();

static inline const char* get_vendor_desc(uint16_t vendor_id) {
  switch (vendor_id) {
    case 0x8086:
      return "Intel Corporation";
    case 0x1234:
      // 0x1234 is the fake vendor id for QEMU:
      // https://github.com/qemu/qemu/blob/eec398119fc6911d99412c37af06a6bc27871f85/include/hw/pci/pci.h#L54 .
      // It's not found on pcisig website though: https://pcisig.com/membership/member-companies
      return "QEMU";
    default:
      break;
  }
  return "unknown vendor";
}

/*
 * Class code is vendor independent. But the interpretation of subclass code
 * depends on class code.
 *
 * The input is the concatenation of class_code and sub_class_code
 * Check pci osdev wiki for interpretations of class code.
 */
static inline const char* get_class_desc(uint16_t full_class_code) {
  switch (full_class_code) {
      // class 0x01: Mass Storge Controller
    case 0x0101:
      return "IDE Controller";
      // class 0x02: Network Controller
    case 0x0200:
      return "Ethernet Controller";
      // class 0x03: Display Controller
    case 0x0300:
      return "VGA Compatible Controller";
      // class 0x06: Bridge
    case 0x0600:
      return "Host Bridge";
    case 0x0601:
      return "ISA Bridge";
    case 0x0680:
      return "Other Bridge";
    default:
      break;
  }
  return "unknown class";
}

class PCIFunction {
 public:
  explicit PCIFunction(uint32_t bus_id, uint32_t dev_id, uint32_t func_id) : bus_id_(bus_id), dev_id_(dev_id), func_id_(func_id) {
  }

  // return true iff the function exists
  operator bool() const {
    return vendor_id() != 0xFFFF;
  }

  /* dump vendor_id, device_id, class_code and subclass_code.
   * Since device_id is vendor specific, this method does not bother to interpret it.
   */
  void dump() const {
    printf("%d:%d.%d", bus_id_, dev_id_, func_id_);

    auto _vendor_id = vendor_id();
    printf(" [vendor] %s (0x%x)", get_vendor_desc(_vendor_id), _vendor_id);

    // device_id
    auto _device_id = device_id();
    printf(" [device] (0x%x)", _device_id);

    // class & subclass code
    auto _class_code = class_code();
    auto _subclass_code = subclass_code();
    auto full_class_code = ((uint16_t) _class_code << 8) | _subclass_code;
    printf("\n  [class] %s (0x%x)", get_class_desc(full_class_code), full_class_code);
    printf("\n");
  }
 public:
  /* APIs handling configuration space */
  uint8_t header_type() const {
    return read_config<uint8_t>(CONFIG_OFF_HEADER_TYPE);
  }

  uint16_t vendor_id() const {
    return read_config<uint16_t>(CONFIG_OFF_VENDOR_ID);
  }

  uint16_t device_id() const {
    return read_config<uint16_t>(CONFIG_OFF_DEVICE_ID);
  }

  uint8_t class_code() const {
    return read_config<uint8_t>(CONFIG_OFF_CLASS_CODE);
  }

  uint8_t subclass_code() const {
    return read_config<uint8_t>(CONFIG_OFF_SUBCLASS_CODE);
  }
 private:
  // the highest byte of the config address should be 0x80
  uint32_t get_config_address(uint8_t offset) const {
    return (0x80 << 24) | (bus_id_ << 16) | (dev_id_ << 11) | (func_id_ << 8) | offset;
  }

  template <typename T>
  T read_config(uint8_t offset) const {
    uint32_t return_size = sizeof(T);
    assert((offset % sizeof(T) == 0) && "unaligned offset");
    assert(return_size > 0 && return_size <= 4);
    assert(((return_size & (return_size - 1)) == 0) && "size should be a power of 2");
    auto addr = get_config_address(offset);

    // TODO make these 2 ports global variables
    Port32Bit addr_port(PORT_CONFIG_ADDRESS);
    Port32Bit data_port(PORT_CONFIG_DATA);

    // make the lowest 2 bits
    addr_port.write(addr & ~0x3);
    // TODO: PCI use little endian. Need do byte swapping for big endian processors
    uint32_t data = data_port.read();
    if (sizeof(T) != sizeof(uint32_t)) {
      data = (data >> ((offset & 0x3) * 8)) & ((1 << (return_size * 8)) - 1);
    }
    return data;
  }
  uint32_t bus_id_;
  uint32_t dev_id_;
  uint32_t func_id_;
};
