#pragma once
/*
 * Driver for Intel 82540EM Gigabit Ethernet Controller.
 */

#include <kernel/nic/nic.h>
#include <string.h>

#define REG_OFF_CTRL 0x0
#define REG_OFF_STATUS 0x8
#define REG_OFF_EERD 0x14
#define REG_OFF_RCTL 0x100
#define REG_OFF_TCTL 0x400

#define REG_OFF_RDBAL 0x2800
#define REG_OFF_RDBAH 0x2804
#define REG_OFF_RDLEN 0x2808
#define REG_OFF_RDH 0x2810
#define REG_OFF_RDT 0x2818

#define REG_OFF_TDBAL 0x3800
#define REG_OFF_TDBAH 0x3804
#define REG_OFF_TDLEN 0x3808
#define REG_OFF_TDH 0x3810
#define REG_OFF_TDT 0x3818

#define REG_OFF_TPR 0x40D0 // total packets received
#define REG_OFF_TPT 0x40D4 // total packets transmitted

#define REG_OFF_RAL0 0x5400
#define REG_OFF_RAH0 0x5404

#define EEPROM_OFF_ENADDR_0 0
#define EEPROM_OFF_ENADDR_1 1
#define EEPROM_OFF_ENADDR_2 2

struct EERD_struct {
  uint32_t start : 1;
  uint32_t reserve_0 : 3;
  uint32_t done : 1;
  uint32_t reserve_1 : 3;
  uint32_t addr : 8;
  uint32_t data : 16;
};
static_assert(sizeof(EERD_struct) == 4);

template <typename REGStruct>
uint32_t regStructToUint32(const REGStruct& regstruct) {
  return *((uint32_t *)(&regstruct));
}

template <typename REGStruct>
REGStruct uint32ToRegStruct(uint32_t val) {
  REGStruct ret;
  memmove(&ret, &val, sizeof(val));
  return ret;
}

class NICDriver_82540EM : public NICDriver {
 public:
  explicit NICDriver_82540EM(const PCIFunction& pci_func = PCIFunction()) : NICDriver(pci_func) { if (pci_func) { init(); } }

  // send the packet synchrouously. T represent the ethernet payload
  template <typename T>
  void sync_send(MACAddr dst_mac_addr, T packet) {
    sync_send(dst_mac_addr, (uint16_t) T::getEtherType(), (uint8_t*) &packet, sizeof(packet));
  }
  // sync_recv is purely used for debugging for now.
  // It wait for at least one incoming packets and then
  // process all available incoming packets before returning.
  void sync_recv();
  // This method returns true iff an ethernet frame is received.
  // If there is no ethernet frame available, return false immediately.
  // The method has side effect: e.g., it will record the gateway MAC address.
  bool unblock_recv();

  void dump_transmit_descriptor_regs();
  void dump_receive_descriptor_regs();

  // the counter may lag a bit
  uint32_t total_packet_transmitted() {
    return read_nic_register(REG_OFF_TPT);
  }
 private:
  void init();
  void init_transmit_descriptor_ring();
  void init_receive_descriptor_ring();

  // 82540EM internal registers are 32 bits size.
  // offset is in byte unit and should align on 4 bytes boundary.
  uint32_t read_nic_register(int off);
  void write_nic_register(int off, uint32_t val);

  // woff is offset of 16bit word
  uint16_t read_eeprom_word(int woff);

  void sync_send(MACAddr dst_mac_addr, uint16_t etherType, uint8_t *data, int len);

  bool has_incoming_frame();

  Bar membar_;
};
