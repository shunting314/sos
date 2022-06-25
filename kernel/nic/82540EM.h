#pragma once
/*
 * Driver for Intel 82540EM Gigabit Ethernet Controller.
 */

#include <kernel/nic/nic.h>
#include <string.h>

#define REG_OFF_EERD 0x14

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
 private:
  void init();

  // 82540EM internal registers are 32 bits size.
  // offset is in byte unit and should align on 4 bytes boundary.
  uint32_t read_nic_register(int off);
  void write_nic_register(int off, uint32_t val);

  // woff is offset of 16bit word
  uint16_t read_eeprom_word(int woff);

  Bar membar_;
};
