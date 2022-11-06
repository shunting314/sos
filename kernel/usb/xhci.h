#pragma once

#include <kernel/usb/hci.h>
#include <kernel/paging.h>
#include <kernel/sleep.h>
#include <assert.h>
#include <string.h>

/*
 * Multi-byte field follows litte endian.
 */

enum TRBType {
  LINK = 6,
};

class TRBTemplate;

class TRBCommon {
 public:
  // clear the TRB to 0 so subclass don't need to worry about it.
  // Assume a TRB has 16 bytes
  explicit TRBCommon() {
    memset(this, 0, 16);
  }
  TRBTemplate toTemplate() const;
};

// template for a transfer request block
class TRBTemplate : public TRBCommon {
 public:
  uint64_t parameter;
  uint32_t status;
  uint32_t c: 1; // cycle bit
  uint32_t ent: 1; // evaluate next TRB
  uint32_t others: 8;
  uint32_t trb_type: 6;
  uint32_t control: 16;
};

static_assert(sizeof(TRBTemplate) == 16);

// an empty class has size 1 rather than 0 so different objects have different
// addresses.
static_assert(sizeof(TRBCommon) == 1);

class LinkTRB : public TRBCommon {
 public:
  explicit LinkTRB(uint32_t segment_addr) {
    ring_segment_pointer_low = (segment_addr >> 4);
    tc = 1; // set toggle bit
    trb_type = TRBType::LINK;
  }

 public:
  uint32_t rsvd0 : 4;
  uint32_t ring_segment_pointer_low : 28;
  uint32_t ring_segment_pointer_high;
  uint32_t rsvd1 : 22;
  uint32_t interrupter_target : 10;
  // cycle bit
  uint32_t c : 1;
  // toggle cycle
  uint32_t tc : 1;
  uint32_t rsvd2 : 2;
  // chain bit
  uint32_t ch : 1;
  uint32_t ioc : 1;
  uint32_t rsvd3 : 4;
  uint32_t trb_type : 6;
  uint32_t rsvd4 : 16;
};

static_assert(sizeof(LinkTRB) == 16);

// Capability register offset
// Refer to the osdev xHCI wiki or xHCI spec.
enum class XHCICapRegOff {
  CAPLENGTH = 0, // 1 byte
  HCSPARAMS1 = 0x04, // 4 byte
  HCCPARAMS1 = 0x10, // capability parameters 1, 4 byte
  DBOFF = 0x14, // 4 byte
};

// All operational registers are multiple of 32 bits in length.
enum class XHCIOpRegOff {
  USBCMD = 0, // USB Comand
  USBSTS = 4, // USB Status
  CRCR_LOW = 0x18, // Command Ring Control Register, lower 4 bytes
  CRCR_HIGH = 0x1C, // Command Ring Control Register, high 4 bytes

  // each port has a group of 4 registers (16 bytes)
  PORTSC = 0x400,
  PORTPMSC = 0x404,
  PORTLI = 0x408,
  PORTHLPMC = 0x40C,
};

struct CRCRegister {
 public:
  CRCRegister(uint32_t ring_addr, uint8_t _rcs)
    : rcs(_rcs), cs(0), ca(0), crr(0), reserved(0) {
    assert((ring_addr & ((1 << 6) - 1)) == 0);
    command_ring_pointer_low = (ring_addr >> 6);
    command_ring_pointer_hi = 0;
  }

  uint32_t getLow32() const {
    return *(uint32_t*) this;
  }

  uint32_t getHigh32() const {
    return command_ring_pointer_hi;
  }

  uint32_t rcs : 1; // ring cycle state. Used to identify the value of the xHC Consumer Cycle State (CSS)
  uint32_t cs : 1; // command stop
  uint32_t ca : 1; // command abort
  uint32_t crr : 1; // command ring running
  uint32_t reserved : 2;

  // defines the initial value of the 64-bit command ring dequeue pointer
  uint32_t command_ring_pointer_low : 26;
  uint32_t command_ring_pointer_hi;
};

static_assert(sizeof(CRCRegister) == 8);

class XHCIDriver : public USBControllerDriver {
 public:
  explicit XHCIDriver(const PCIFunction& pci_func = PCIFunction()) : USBControllerDriver(pci_func) {
    if (pci_func_) {
      membar_ = setupMembar();
    }
  }

  // refer to xHCI spec 4.2 for the initialization process.
  void reset() {
    printf("CAPLENGTH %d\n", getCapLength());
    printf("DBOFF %d\n", getDBOff());
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

    printf("HCSPARAMS1 0x%x\n", getHCSParams1());
    printf("HCCPARAMS1 0x%x\n", getHCCParams1());
    // assume CSZ == 0 indicting 32 byte context structure size
    assert((getHCCParams1() & (1 << 2)) == 0);
    printf("USBCMD 0x%x\n", getUSBCmd());
    printf("USBSTS 0x%x\n", getUSBSts());
    printf("CRCR_LOW 0x%x\n", getCRCRLow());
    printf("CRCR_HIGH 0x%x\n", getCRCRHigh());

    assert(getUSBSts() & 0x1); // halted

    // reset
    setUSBCmdFlags(1 << 1); // set the HCRST bit
    while (getUSBCmd() & (1 << 1)) {
      msleep(1);
    }
    // the controller will clear the HCRST bit when the reset is done
    assert((getUSBCmd() & (1 << 1)) == 0);
    printf("Max slots %d\n", getMaxSlots());
    printf("Max interrupters %d\n", getMaxInterrupters());
    printf("Max port %d\n", getMaxPort());
    for (int port_no = 1; port_no <= getMaxPort(); ++port_no) {
      printf("port %d sc 0x%x\n", port_no, getPortSC(port_no));
    }

    // TODO reset the port. Check xHCI spec 4.19.5

    initialize();
  }

  void initialize();

  // API for capability registers

  uint8_t getCapLength() {
    return *(uint8_t*) getCapRegPtr(XHCICapRegOff::CAPLENGTH);
  }

  // return the max port number. Note that port number starts from 1.
  // So max port number equals the total number of available ports.
  uint32_t getMaxPort() {
    return getHCSParams1() >> 24;
  }

  uint32_t getMaxInterrupters() {
    return (getHCSParams1() >> 8) & 0x7FF; // bit 8..18
  }

  uint32_t getMaxSlots() {
    return getHCSParams1() & 0xFF;
  }

  uint32_t getHCSParams1() {
    return *(uint32_t*) getCapRegPtr(XHCICapRegOff::HCSPARAMS1);
  }

  uint32_t getHCCParams1() {
    return *(uint32_t*) getCapRegPtr(XHCICapRegOff::HCCPARAMS1);
  }

  uint32_t getDBOff() {
    return *(uint32_t*) getCapRegPtr(XHCICapRegOff::DBOFF);
  }

  // API for operational registers

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

  uint32_t getCRCRLow() {
    return *getOpRegPtr(XHCIOpRegOff::CRCR_LOW);
  }

  void setCRCRLow(uint32_t low) {
    *getOpRegPtr(XHCIOpRegOff::CRCR_LOW) = low;   
  }

  uint32_t getCRCRHigh() {
    return *getOpRegPtr(XHCIOpRegOff::CRCR_HIGH);
  }

  void setCRCRHigh(uint32_t high) {
    *getOpRegPtr(XHCIOpRegOff::CRCR_HIGH) = high;
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
