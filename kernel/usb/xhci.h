#pragma once

#include <kernel/usb/hci.h>
#include <kernel/paging.h>
#include <assert.h>

/*
 * Multi-byte field follows litte endian.
 */

// Capability register offset
// Refer to the osdev xHCI wiki or xHCI spec.
enum class XHCICapRegOff {
  CAPLENGTH = 0, // 1 byte
  HCSPARAMS1 = 0x04, // 4 byte
};

// All operational registers are multiple of 32 bits in length.
enum class XHCIOpRegOff {
  USBCMD = 0, // USB Comand
  USBSTS = 4, // USB Status
  // each port has a group of 4 registers (16 bytes)
  PORTSC = 0x400,
  PORTPMSC = 0x404,
  PORTLI = 0x408,
  PORTHLPMC = 0x40C,
};

class XHCIDriver : public USBControllerDriver {
 public:
  explicit XHCIDriver(const PCIFunction& pci_func = PCIFunction()) : USBControllerDriver(pci_func) {
    if (pci_func_) {
      membar_ = setupMembar();
    }
  }

  void reset() {
    printf("CAPLENGTH %d\n", getCapLength());
    opRegBase_ = (void *) membar_.get_addr() + getCapLength();

    // the controller is in a running state if a usb drive is attached at the
    // beginning. Stop it first.
    if (getUSBCmd() & 0x1) {
      setUSBCmd(getUSBCmd() & ~0x1);
      // wait until the HCHalted bit is set
      while ((getUSBSts() & 0x1) == 0) {
        msleep(1);
      }
    }

    printf("USBCMD 0x%x\n", getUSBCmd());
    printf("USBSTS 0x%x\n", getUSBSts());
    assert(getUSBSts() & 0x1); // halted

    // reset
    setUSBCmdFlags(1 << 1); // set the HCRST bit
    while (getUSBCmd() & (1 << 1)) {
      msleep(1);
    }
    // the controller will clear the HCRST bit when the reset is done
    assert((getUSBCmd() & (1 << 1)) == 0);
    printf("Max port %d\n", getMaxPort());
    for (int port_no = 1; port_no <= getMaxPort(); ++port_no) {
      printf("port %d sc 0x%x\n", port_no, getPortSC(port_no));
    }

    #if 0
    // TODO reset the port.
    assert(false && "reset ni");
    #endif
  }

  uint8_t getCapLength() {
    return *(uint8_t*) getCapRegPtr(XHCICapRegOff::CAPLENGTH);
  }

  // return the max port number. Note that port number starts from 1.
  // So max port number equals the total number of available ports.
  uint32_t getMaxPort() {
    return getHCSParams1() >> 24;
  }

  uint32_t getHCSParams1() {
    return *(uint32_t*) getCapRegPtr(XHCICapRegOff::HCSPARAMS1);
  }

  uint32_t getUSBCmd() {
    return *getOpRegPtr(XHCIOpRegOff::USBCMD);
  }

  void setUSBCmdFlags(uint32_t flags) {
    setUSBCmd(getUSBCmd() | flags);
  }

  void setUSBCmd(uint32_t cmd) {
    *getOpRegPtr(XHCIOpRegOff::USBCMD) = cmd;
  }

  uint32_t getUSBSts() {
    return *getOpRegPtr(XHCIOpRegOff::USBSTS);
  }

  // port_no starts from 1
  uint32_t getPortSC(uint32_t port_no) {
    return *(getOpRegPtr(XHCIOpRegOff::PORTSC) + 4 * (port_no - 1));
  }
 private:
  void* getCapRegPtr(XHCICapRegOff off) {
    return (void *) (membar_.get_addr() + (int) off);
  }

  uint32_t* getOpRegPtr(XHCIOpRegOff off) {
    return (uint32_t *) (opRegBase_ + (int) off);
  }

 private:
  Bar membar_;
  void *opRegBase_;
};
