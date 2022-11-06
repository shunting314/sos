#include <kernel/usb/xhci.h>

TRBTemplate TRBCommon::toTemplate() const {
  return *(TRBTemplate*) this;
}

// All TRBBase on the list contains all 0 initially. This is archieved by the
// TRBCommon ctor.
class TRBRing {
 public:
  explicit TRBRing() {
    assert((get_addr() & 0xFFF) == 0); // page alignment
    assert(sizeof(trb_ring_) == 4096);

    trb_ring_[capacity() - 1] = LinkTRB(get_addr()).toTemplate();
  }

  uint32_t capacity() const {
    return sizeof(trb_ring_) / sizeof(*trb_ring_);
  }

  uint32_t get_addr() const {
    return (uint32_t) &trb_ring_;
  }
 private:
  // 256 items
  TRBTemplate trb_ring_[4096 / sizeof(TRBTemplate)] __attribute__((aligned(4096))); 
};

TRBRing command_ring;

void XHCIDriver::initialize() {
  // initialize the command ring. Setup CRCR to point to it.

  // set RCS to 1 so the C bit of all TRBS on the command ring
  // can be intialized to 0
  CRCRegister crcc(command_ring.get_addr(), 1 /* rcs */);
  setCRCRLow(crcc.getLow32());
  setCRCRHigh(crcc.getHigh32());

  assert(getCRCRHigh() == 0);
  // the equality holds since the lowest 6 bits of CRCR are all 0
  assert((getCRCRLow() & ~63)  == (uint32_t) &command_ring);

  assert(false && "initialize ni");
}
