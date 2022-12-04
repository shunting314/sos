#pragma once

#include <kernel/usb/hci.h>
#include <kernel/usb/xhci_ctx.h>
#include <kernel/usb/xhci_trb.h>
#include <kernel/usb/usb_proto.h>
#include <kernel/paging.h>
#include <kernel/sleep.h>
#include <assert.h>
#include <string.h>

/*
 * Multi-byte field follows litte endian.
 */

// Capability register offset
// Refer to the osdev xHCI wiki or xHCI spec.
enum class XHCICapRegOff {
  CAPLENGTH = 0, // 1 byte
  HCSPARAMS1 = 0x04, // structural params 1, 4 byte
  HCSPARAMS2 = 0x08, // structural params 2, 4 byte
  HCCPARAMS1 = 0x10, // capability parameters 1, 4 byte
  DBOFF = 0x14, // 4 byte
  RTSOFF = 0x18, // runtime reigster space offset, 4 bytes.
};

// All operational registers are multiple of 32 bits in length.
enum class XHCIOpRegOff {
  USBCMD = 0, // USB Comand
  USBSTS = 4, // USB Status
  CRCR_LOW = 0x18, // Command Ring Control Register, lower 4 bytes
  CRCR_HIGH = 0x1C, // Command Ring Control Register, high 4 bytes
  DCBAAP_LOW = 0x30, // Device context base address array pointer
  DCBAAP_HIGH = 0x34, // Device context base address array pointer
  CONFIG = 0x38, // configure register, 32 bits

  // each port has a group of 4 registers (16 bytes)
  PORTSC = 0x400,
  PORTPMSC = 0x404,
  PORTLI = 0x408,
  PORTHLPMC = 0x40C,
};

enum class XHCIRuntimeRegOff {
  IMAN = 0x20, // interrupter management register, 4 bytes
  IMOD = 0x24, // interrupter moderation register, 4 bytes
  // event ring segment table size register, 4 bytes
  // define the nubmer of event ring segments
  ERSTSZ = 0x28,
  // event ring segment table base address register
  ERSTBA_LOW = 0x30, 
  ERSTBA_HIGH = 0x34,
  // event ring dequeue pointer register.
  // Software updates the pointer when it finishes the evaluation
  // of an event on the event ring.
  ERDP_LOW = 0x38,
  ERDP_HIGH = 0x3c,
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

class TRBRing;

class EventRingSegmentTableEntry {
 public:
  explicit EventRingSegmentTableEntry(const TRBRing& trb_ring);
  explicit EventRingSegmentTableEntry() { }
  // low 6 bits should be 0. i.e., segment table should be 64 byte align.
  uint32_t ring_segment_base_address_low;
  uint32_t ring_segment_base_address_high;
  // high 16 bits all 0.
  // Count in number of TRBs. Value values should be in [16, 4096]
  uint32_t ring_segment_size;
  uint32_t rsvd;
};

static_assert(sizeof(EventRingSegmentTableEntry) == 16);

template <typename ControllerDriver>
class USBDevice;

class XHCIDriver : public USBControllerDriver {
 public:
  explicit XHCIDriver(const PCIFunction& pci_func = PCIFunction()) : USBControllerDriver(pci_func) {
    if (pci_func_) {
      membar_ = setupMembar();
    }
  }

  // APIs talking to USB devices
  void sendDeviceRequest(USBDevice<XHCIDriver>* device, DeviceRequest* req, void *buf);

  // refer to xHCI spec 4.2 for the initialization process.
  void reset() {
    printf("CAPLENGTH %d\n", getCapLength());
    printf("DBOFF %d\n", getDBOff());
    printf("RTSOFF %d\n", getRTSOff());
    opRegBase_ = (void *) membar_.get_addr() + getCapLength();
    runtimeRegBase_ = (void *) membar_.get_addr() + getRTSOff();

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
    printf("HCSPARAMS2 0x%x\n", getHCSParams2());
    printf("HCCPARAMS1 0x%x\n", getHCCParams1());
    // assume CSZ == 0 indicting 32 byte context structure size
    assert((getHCCParams1() & (1 << 2)) == 0);
    printf("USBCMD 0x%x\n", getUSBCmd());
    printf("USBSTS 0x%x\n", getUSBSts());
    printf("CRCR_LOW 0x%x\n", getCRCRLow());
    printf("CRCR_HIGH 0x%x\n", getCRCRHigh());
    printf("DCBAAP_LOW 0x%x\n", getDCBAAPLow());
    printf("DCBAAP_HIGH 0x%x\n", getDCBAAPHigh());
    printf("CONFIG 0x%x\n", getConfig());

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

    initialize();
    setUSBCmdFlags(1 << 0); // set the run/stop bit
    while (getUSBSts() & 0x1) {
      msleep(1);
    }
    assert((getUSBSts() & 0x1) == 0); // not halted

    // resetting the root hub port will reset the attached device
    resetPort(1);
  }
 
  void initializeDevice(USBDevice<XHCIDriver>* dev);
  void resetPort(int port_no);

  void initializeContext();
  void initializeCommandRing();
  void initializeInterrupter();
  void initialize();
  uint32_t allocate_device_slot();
  uint32_t assign_usb_device_address(uint32_t slot_id);

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

  uint32_t getHCSParams2() {
    return *(uint32_t*) getCapRegPtr(XHCICapRegOff::HCSPARAMS2);
  }

  uint32_t getHCCParams1() {
    return *(uint32_t*) getCapRegPtr(XHCICapRegOff::HCCPARAMS1);
  }

  // get extended capabilities pointer. 0 means no extended capabilities
  // If non zero, this pointer specifies the offset to membar base in dword
  // unit.
  uint32_t getECP() {
    return getHCCParams1() >> 16;
  }

  uint32_t getDBOff() {
    return *(uint32_t*) getCapRegPtr(XHCICapRegOff::DBOFF);
  }

  /*
   * xhci spec 5.3.7 mentions DBOFF defines the offset in *Dwords* of the doorbell
   * array base addres from the Base.
   *
   * However in practice, there is no need to multiply DBOFF by 4 to calculate the
   * address of the doorbell array.
   */
  void ringDoorbell(uint32_t index, uint32_t val) {
    assert(index <= 255);
    volatile uint32_t* addr = (volatile uint32_t*) (membar_.get_addr() + getDBOff());
    addr[index] = val;
  }

  uint32_t getRTSOff() {
    return *(uint32_t*) getCapRegPtr(XHCICapRegOff::RTSOFF);
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

  uint32_t getDCBAAPLow() {
    return *getOpRegPtr(XHCIOpRegOff::DCBAAP_LOW);
  }

  void setDCBAAPLow(uint32_t low) {
    assert((low & 63) == 0); // 64 byte aligh. Check xhci spec 5.4.6
    *getOpRegPtr(XHCIOpRegOff::DCBAAP_LOW) = low;
  }

  uint32_t getDCBAAPHigh() {
    return *getOpRegPtr(XHCIOpRegOff::DCBAAP_HIGH);
  }

  void setDCBAAPHigh(uint32_t high) {
    *getOpRegPtr(XHCIOpRegOff::DCBAAP_HIGH) = high;
  }

  uint32_t getConfig() {
    return *getOpRegPtr(XHCIOpRegOff::CONFIG);
  }

  // port_no starts from 1
  uint32_t getPortSC(uint32_t port_no) {
    return *(getOpRegPtr(XHCIOpRegOff::PORTSC) + 4 * (port_no - 1));
  }

  void setPortSC(uint32_t port_no, uint32_t sc) {
    *(getOpRegPtr(XHCIOpRegOff::PORTSC) + 4 * (port_no - 1)) = sc;
  }

  void setPortSCFlags(uint32_t port_no, uint32_t flags) {
    setPortSC(port_no, getPortSC(port_no) | flags);
  }

  // API for runtime registers
  #define INTERRUPTER_REGISTER_REGION_SIZE 32
  uint32_t getIMan(uint32_t interrupter_id) {
    return *(uint32_t*) ((uint8_t*) getRuntimeRegPtr(XHCIRuntimeRegOff::IMAN) + interrupter_id * INTERRUPTER_REGISTER_REGION_SIZE);
  }

  uint32_t getIMod(uint32_t interrupter_id) {
    return *(uint32_t*) ((uint8_t*) getRuntimeRegPtr(XHCIRuntimeRegOff::IMOD) + interrupter_id * INTERRUPTER_REGISTER_REGION_SIZE);
  }

  uint32_t getERSTSZ(uint32_t interrupter_id) {
    return *(uint32_t*) ((uint8_t*) getRuntimeRegPtr(XHCIRuntimeRegOff::ERSTSZ) + interrupter_id * INTERRUPTER_REGISTER_REGION_SIZE);
  }

  void setERSTSZ(uint32_t interrupter_id, int sz) {
    *(uint32_t*) ((uint8_t*) getRuntimeRegPtr(XHCIRuntimeRegOff::ERSTSZ) + interrupter_id * INTERRUPTER_REGISTER_REGION_SIZE) = sz;
  }

  void setERSTBALow(uint32_t interrupter_id, uint32_t low) {
    assert((low & 63) == 0);
    *(uint32_t*) ((uint8_t*) getRuntimeRegPtr(XHCIRuntimeRegOff::ERSTBA_LOW) + interrupter_id * INTERRUPTER_REGISTER_REGION_SIZE) = low;
  }

  void setERSTBAHigh(uint32_t interrupter_id, uint32_t high) {
    *(uint32_t*) ((uint8_t*) getRuntimeRegPtr(XHCIRuntimeRegOff::ERSTBA_HIGH) + interrupter_id * INTERRUPTER_REGISTER_REGION_SIZE) = high;
  }

  void setERDPLow(uint32_t interrupter_id, uint32_t low) {
    assert((low & 0xF) == 0);
    *(uint32_t*) ((uint8_t*) getRuntimeRegPtr(XHCIRuntimeRegOff::ERDP_LOW) + interrupter_id * INTERRUPTER_REGISTER_REGION_SIZE) = low;
  }

  void setERDPHigh(uint32_t interrupter_id, uint32_t high) {
    *(uint32_t*) ((uint8_t*) getRuntimeRegPtr(XHCIRuntimeRegOff::ERDP_HIGH) + interrupter_id * INTERRUPTER_REGISTER_REGION_SIZE) = high;
  }
 private:
  void* getCapRegPtr(XHCICapRegOff off) {
    return (void *) (membar_.get_addr() + (int) off);
  }

  uint32_t* getOpRegPtr(XHCIOpRegOff off) {
    return (uint32_t *) (opRegBase_ + (int) off);
  }

  uint32_t* getRuntimeRegPtr(XHCIRuntimeRegOff off) {
    return (uint32_t *) (runtimeRegBase_ + (int) off);
  }

 private:
  Bar membar_;
  void *opRegBase_;
  void *runtimeRegBase_;
};
