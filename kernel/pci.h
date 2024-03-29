#pragma once
#include <assert.h>
#include <stdint.h>
#include <kernel/ioport.h>
#include <kernel/pci_ids.h>

#define PORT_CONFIG_ADDRESS 0xCF8
#define PORT_CONFIG_DATA 0xCFC
extern Port32Bit pci_addr_port;
extern Port32Bit pci_data_port;

#define MAX_NUM_BUS 256 // max number of pci buses
#define MAX_NUM_DEVICES_PER_BUS 32 // max number of devices per bus
#define MAX_NUM_FUNCTIONS_PER_DEVICE 8 // max number of functions per device

#define COMMAND_STATUS_BUS_MASTER_BIT 2

// check PCI osdev wiki
#define CONFIG_OFF_VENDOR_ID 0x0 // 2 byte size
#define CONFIG_OFF_DEVICE_ID 0x2 // 2 byte size
#define CONFIG_OFF_COMMAND_STATUS 0x4
#define CONFIG_OFF_PROG_IF 0x9 // 1 byte size
#define CONFIG_OFF_SUBCLASS_CODE 0xA // 1 byte size
#define CONFIG_OFF_CLASS_CODE 0xB // 1 byte size
#define CONFIG_OFF_HEADER_TYPE 0xE // 1 byte size
#define CONFIG_OFF_BAR0 0x10 // 4 byte size
#define CONFIG_OFF_BAR1 0x14 // 4 byte size
#define CONFIG_OFF_BAR2 0x18 // 4 byte size
#define CONFIG_OFF_BAR3 0x1C // 4 byte size
#define CONFIG_OFF_BAR4 0x20 // 4 byte size
#define CONFIG_OFF_BAR5 0x14 // 4 byte size
#define CONFIG_MAX_NUM_BARS 6

#define CONFIG_OFF_INTERRUPT_LINE 0x3C
#define CONFIG_OFF_INTERRUPT_PIN 0x3D

#define IOBAR_INFO_BITS_MASK 0x3
#define MEMBAR_INFO_BITS_MASK 0xF

/*
 * Visit all available pci devices and print each's metadata.
 */
void lspci();

/*
 * Visit all available pci devices and record the PCIFunction for the
 * interested ones.
 */
void collect_pci_devices();

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

enum class FullClassCode {
  IDE_CONTROLLER = 0x0101,
  ETHERNET_CONTROLLER = 0x0200,
  VGA_COMPATIBLE_CONTROLLER = 0x0300,
  HOST_BRIDGE = 0x0600,
  ISA_BRIDGE = 0x0601,
  OTHER_BRIDGE = 0x0680,
  USB_CONTROLLER = 0xC03,
};

/*
 * Class code is vendor independent. But the interpretation of subclass code
 * depends on class code.
 *
 * The input is the concatenation of class_code and sub_class_code
 * Check pci osdev wiki for interpretations of class code.
 */
static inline const char* get_class_desc(uint16_t full_class_code) {
  switch ((FullClassCode) full_class_code) {
      // class 0x01: Mass Storge Controller
    case FullClassCode::IDE_CONTROLLER:
      return "IDE Controller";
      // class 0x02: Network Controller
    case FullClassCode::ETHERNET_CONTROLLER:
      return "Ethernet Controller";
      // class 0x03: Display Controller
    case FullClassCode::VGA_COMPATIBLE_CONTROLLER:
      return "VGA Compatible Controller";
      // class 0x06: Bridge
    case FullClassCode::HOST_BRIDGE:
      return "Host Bridge";
    case FullClassCode::ISA_BRIDGE:
      return "ISA Bridge";
    case FullClassCode::OTHER_BRIDGE:
      return "Other Bridge";
      // class 0x0C: Serial Bus Controller
    case FullClassCode::USB_CONTROLLER:
      return "USB Controlelr";
    default:
      break;
  }
  return "unknown class";
}

// base address register
class Bar {
 public:
  operator bool() const {
    // raw == 0 indicas a non-exist BAR
    return idx_ >= 0 && raw_ != 0;
  }

  void print() const {
    printf("idx %d, addr 0x%x, size %d (0x%x), is_mem %d, raw 0x%x\n", idx_, addr_, size_, size_, is_mem_, raw_);
  }

  void set_idx(int idx) { idx_ = idx; }
  int get_idx() const { return idx_; }

  void set_raw(uint32_t raw) {
    raw_ = raw;
    is_mem_ = (raw & 1) == 0;
    addr_ = clear_info_bits(raw);
  }
  uint32_t get_raw() const { return raw_; }
  uint32_t get_addr() const { return addr_; }

  // need clear the information bits in raw_size and then negate
  void set_size(uint32_t raw_size) {
    size_ = -clear_info_bits(raw_size);
  }

  // Need access the memory region size so we can create vitual address mapping
  uint32_t get_size() const {
    return size_;
  }

  bool isMem() const {
    return is_mem_;
  }

  bool isIO() const {
    return !isMem();
  }
 private:
  uint32_t clear_info_bits(uint32_t raw) {
    if (is_mem_) {
      return raw & ~MEMBAR_INFO_BITS_MASK;
    } else {
      return raw & ~IOBAR_INFO_BITS_MASK;
    }
  }

  int idx_ = -1; // indicate an uninitialized state
  uint32_t size_ = 0;
  bool is_mem_ = 0; // mem or IO
  uint32_t raw_ = 0; // raw content containing start address and information bits
  uint32_t addr_ = 0;
};

class PCIFunction {
 public:
  explicit PCIFunction(uint32_t bus_id=0xFFFF, uint32_t dev_id=0, uint32_t func_id=0) : bus_id_(bus_id), dev_id_(dev_id), func_id_(func_id) {
  }

  // return true iff the function exists
  operator bool() const {
    return bus_id_ != 0xFFFF && vendor_id() != 0xFFFF;
  }

  /* dump vendor_id, device_id, class_code and subclass_code.
   * Since device_id is vendor specific, this method does not bother to interpret it.
   */
  void dump() const;

  Bar getBar(int idx) const;
  // succeed only if the device has a single valid Bar and return that Bar.
  // assert fail otherwise.
  Bar getSoleBar() const;
  // return a 2 bytes word whose MSB is the class code and LSB is the subclass code.
  uint16_t full_class_code() const {
    auto _class_code = class_code();
    auto _subclass_code = subclass_code();
    return ((uint16_t) _class_code << 8) | _subclass_code;
  }

  void dumpBars() const {
    for (int i = 0; i < 6; ++i) {
      getBar(i).print();
    }
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

  uint8_t prog_if() const {
    return read_config<uint8_t>(CONFIG_OFF_PROG_IF);
  }

  // number between 0-15
  uint8_t interrupt_line() const {
    return read_config<uint8_t>(CONFIG_OFF_INTERRUPT_LINE);
  }

  uint8_t interrupt_pin() const {
    return read_config<uint8_t>(CONFIG_OFF_INTERRUPT_PIN);
  }

  uint32_t bar(uint8_t idx) const {
    // TODO pci-to-pci bridge device has less BARs
    assert(idx < 6);
    return read_config<uint32_t>(CONFIG_OFF_BAR0 + idx * 4);
  }

  void set_bar(uint8_t idx, uint32_t new_bar) const {
    // TODO pci-to-pci bridge device has less BARs
    assert(idx < 6);
    write_config<uint32_t>(CONFIG_OFF_BAR0 + idx * 4, new_bar);
  }

  /*
   * Enabling the bus master bit is necessary for the device to do DMA
   */
  void enable_bus_master() const {
    uint32_t oldval = read_config<uint32_t>(CONFIG_OFF_COMMAND_STATUS);
    write_config<uint32_t>(CONFIG_OFF_COMMAND_STATUS, oldval | (1 << COMMAND_STATUS_BUS_MASTER_BIT));
  }

  uint32_t get_command_status() const {
    return read_config<uint32_t>(CONFIG_OFF_COMMAND_STATUS);
  }

  template <typename T>
  void write_config(uint8_t offset, uint32_t newval) const {
    uint32_t item_size = sizeof(T);
    assert((offset % sizeof(T) == 0) && "unaligned offset");
    assert(item_size > 0 && item_size <= 4);
    assert(((item_size & (item_size - 1)) == 0) && "size should be a power of 2");
    auto addr = get_config_address(offset);


    if (item_size == 4) {
      // easy case
      // mask the lowest 2 bits
      assert((addr & 3) == 0);
      pci_addr_port.write(addr);
      pci_data_port.write(newval);
    } else {
      uint32_t oldval = read_config<uint32_t>(offset & ~0x3);

      // calculate the mask & shift for the value to set
      int shift = ((offset & 3) << 3);
      int nbits = (item_size << 3);
      uint32_t mask = ((1 << nbits) - 1);
      uint32_t shifted_mask = (mask << shift);

      assert((newval & ~mask) == 0);
      uint32_t finalval = (oldval & ~shifted_mask) | (newval << shift);

      // mask the lowest 2 bits
      pci_addr_port.write(addr & ~0x3);
      pci_data_port.write(finalval);
    }
  }

  template <typename T>
  T read_config(uint8_t offset) const {
    uint32_t return_size = sizeof(T);
    assert((offset % sizeof(T) == 0) && "unaligned offset");
    assert(return_size > 0 && return_size <= 4);
    assert(((return_size & (return_size - 1)) == 0) && "size should be a power of 2");
    auto addr = get_config_address(offset);

    // mask the lowest 2 bits
    pci_addr_port.write(addr & ~0x3);
    // TODO: PCI use little endian. Need do byte swapping for big endian processors
    uint32_t data = pci_data_port.read();
    if (sizeof(T) != sizeof(uint32_t)) {
      data = (data >> ((offset & 0x3) * 8)) & ((1 << (return_size * 8)) - 1);
    }
    return data;
  }
 private:
  // the highest byte of the config address should be 0x80
  uint32_t get_config_address(uint8_t offset) const {
    return (0x80 << 24) | (bus_id_ << 16) | (dev_id_ << 11) | (func_id_ << 8) | offset;
  }

  uint32_t bus_id_;
  uint32_t dev_id_;
  uint32_t func_id_;
};
