#pragma once

#include <kernel/pci.h>
#include <kernel/sleep.h>
#include <kernel/phys_page.h>
#include <kernel/usb/usb_proto.h>
#include <string.h>

enum class UHCIRegOff {
  CMD = 0, // command, 2 bytes
  STS = 2, // status, 2 bytes
  FRNUM = 6, // frame number, 2 bytes
  FLBASEADDR = 8, // frame list base address, 4 bytes. page align (i.e. low 12 bits all 0).
  SOFMOD = 12, // SOF modify, 1 byte
  PORTSC1 = 16, // port 1 status/control, 2 bytes
  PORTSC2 = 18, // port 2 status/control, 2 bytes
};

class USBDevice;

class UHCIDriver {
 public:
  explicit UHCIDriver(const PCIFunction& pci_func = PCIFunction()) : pci_func_(pci_func) {
    if (pci_func_) {
      printf("UHCI interrupt line %d, interrupt pin %d\n", pci_func_.interrupt_line(), pci_func_.interrupt_pin());
      setupBar();
    }
  }

  void setupFramePtrs();

  void resetPort(Port16Bit ioPort) {
    // refer to the usb book about how to reset UHCI root hub ports.
    // Note: UHCI root hub ports must be reset separately even though
    // a global reset has already been done.
    ioPort.write(ioPort.read() | (1 << 9)); // set the reset bit
    msleep(50); // wait for 50 ms
    ioPort.write(ioPort.read() & ~(1 << 9)); // clear the reset bit

    // clear the connect status change bit. Bit a write clear bit, so we
    // have to write a 1 to clear it
    ioPort.write(ioPort.read() | (1 << 1));

    // set the enable bit
    ioPort.write(ioPort.read() | (1 << 2));
    assert(ioPort.read() & (1 << 2)); // make sure the port is enabled
  }

  void reset() {
    assert(!(getCmd() & 1) && (getStatus() & (1 << 5)) && "The controller should be in a halted state");
    setCmdFlags(1 << 2); // global reset
    msleep(10); // wait for 10 ms
    clearCmdFlags(1 << 2); // reset global reset flag

    // setup the frame list
    frame_page_addr_ = alloc_phys_page();
    memset((void*) frame_page_addr_, 0, 4096);
    setupFramePtrs();
    setFrameListBaseAddr(frame_page_addr_);

    // start the controller
    setCmdFlags(1 << 0);
    assert(!(getStatus() & (1 << 5)) && "The controller halted flag should be off");

    // assume that port1 has device attached but port2 does not.
    // TODO: we should have the ability to detect device dynamically
    assert(getSC1() == 0x83); // bit 7 is reserved and should always be 1
    assert(getSC2() == 0x80);
    resetPort(sc1_port_);
  }

  uint16_t getCmd() {
    return cmd_port_.read();
  }

  // only clear the specified flags
  void clearCmdFlags(uint16_t flags) {
    setCmd(getCmd() & ~flags);
  }

  // only set the specified flags
  void setCmdFlags(uint16_t flags) {
    setCmd(getCmd() | flags);
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

  void setFrnum(uint16_t frnum) {
    frnum_port_.write(frnum);
  }

  void setFrameListBaseAddr(uint32_t frameListBaseAddr) {
    flbaseaddr_port_.write(frameListBaseAddr);
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

 public:
  // APIs talking to USB devices
  void sendDeviceRequest(USBDevice* device, DeviceRequest* req, void *buf);

  // a simple implementation that only do allocation but no reclamation
  uint8_t acquireAvailAddr() const {
    static uint8_t next_addr = 1;
    assert(next_addr > 0 && next_addr <= 127 && "No more address available");
    return next_addr++;
  }
 private:
  void setupBar() {
    iobar_ = pci_func_.getSoleBar();
    assert(iobar_.isIO());
    iobar_.print();

    cmd_port_ = Port16Bit(iobar_.get_addr() + (int) UHCIRegOff::CMD);
    status_port_ = Port16Bit(iobar_.get_addr() + (int) UHCIRegOff::STS);
    flbaseaddr_port_ = Port32Bit(iobar_.get_addr() + (int) UHCIRegOff::FLBASEADDR);
    frnum_port_ = Port16Bit(iobar_.get_addr() + (int) UHCIRegOff::FRNUM);
    sofmod_port_ = Port8Bit(iobar_.get_addr() + (int) UHCIRegOff::SOFMOD);
    sc1_port_ = Port16Bit(iobar_.get_addr() + (int) UHCIRegOff::PORTSC1);
    sc2_port_ = Port16Bit(iobar_.get_addr() + (int) UHCIRegOff::PORTSC2);
  }

  PCIFunction pci_func_;
  Bar iobar_;

  Port16Bit cmd_port_;
  Port16Bit status_port_;
  Port32Bit flbaseaddr_port_;
  Port16Bit frnum_port_;
  Port8Bit sofmod_port_;
  Port16Bit sc1_port_;
  Port16Bit sc2_port_;

  uint32_t frame_page_addr_;
};
