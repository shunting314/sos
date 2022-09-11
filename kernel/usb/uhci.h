#pragma once

#include <kernel/pci.h>

enum class UHCIRegOff {
  CMD = 0, // command, 2 bytes
  STS = 2, // status, 2 bytes
  FRNUM = 6, // frame number, 2 bytes
  FLBASEADDR = 8, // frame list base address, 4 bytes. page align (i.e. low 12 bits all 0).
  SOFMOD = 12, // SOF modify, 1 byte
  PORTSC1 = 16, // port 1 status/control, 2 bytes
  PORTSC2 = 18, // port 2 status/control, 2 bytes
};

class UHCIDriver {
 public:
  explicit UHCIDriver(const PCIFunction& pci_func = PCIFunction()) : pci_func_(pci_func) {
    if (pci_func_) {
      printf("UHCI interrupt line %d, interrupt pin %d\n", pci_func_.interrupt_line(), pci_func_.interrupt_pin());
      setupBar();
    }
  }

  void reset() {
    printf("STS 0x%x\n", getStatus());
    printf("SOFMOD 0x%x\n", getSOFMod());
    printf("SC1 0x%x\n", getSC1());
    printf("SC2 0x%x\n", getSC2());
    printf("UHCIDriver reset not fully implemented yet\n");
  }

  uint16_t getCmd() {
    return cmd_port_.read();
  }

  void setCmd(uint16_t cmd) {
    cmd_port_.write(cmd);
  }

  uint16_t getStatus() {
    return status_port_.read();
  }

  void setStatus(uint16_t status) {
    status_port_.write(status);
  }

  uint8_t getSOFMod() {
    return sofmod_port_.read();
  }

  void setSOFMod(uint8_t sofmod) {
    sofmod_port_.write(sofmod);
  }

  uint16_t getSC1() {
    return sc1_port_.read();
  }

  uint16_t getSC2() {
    return sc2_port_.read();
  }
 private:
  void setupBar() {
    iobar_ = pci_func_.getSoleBar();
    assert(iobar_.isIO());
    iobar_.print();

    cmd_port_ = Port16Bit(iobar_.get_addr() + (int) UHCIRegOff::CMD);
    status_port_ = Port16Bit(iobar_.get_addr() + (int) UHCIRegOff::STS);
    sofmod_port_ = Port8Bit(iobar_.get_addr() + (int) UHCIRegOff::SOFMOD);
    sc1_port_ = Port16Bit(iobar_.get_addr() + (int) UHCIRegOff::PORTSC1);
    sc2_port_ = Port16Bit(iobar_.get_addr() + (int) UHCIRegOff::PORTSC2);
  }

  PCIFunction pci_func_;
  Bar iobar_;

  Port16Bit cmd_port_;
  Port16Bit status_port_;
  Port8Bit sofmod_port_;
  Port16Bit sc1_port_;
  Port16Bit sc2_port_;
};
