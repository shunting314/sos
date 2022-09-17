#include <kernel/usb/uhci.h>

/*
 * The current schedule implemented for the UHCIDriver is very simple.
 * All the frame pointers point to the same queue. The queue points to
 * a sequence of TransferDescriptor's. TODO: we may need to improve it
 * in future.
 */

/*
 * Used either as a frame ptr, or inside a TransferDescriptor or inside a queue.
 */
class GenericPtr {
 public:
  // use the default args to create an null pointer
  explicit GenericPtr(uint32_t addr = 0, bool isQueue = 0, bool isTerm = 1, bool isDepthFirst=false) {
    assert((addr & 0xF) == 0);
    addr28_ = (addr >> 4);
    reserved_ = 0;
    isDepthFirst_ = isDepthFirst;
    isQueue_ = isQueue;
    isTerm_ = isTerm;
  }

  static GenericPtr createQueuePtr(uint32_t addr) {
    return GenericPtr(addr, 1 /* isQueue */, 0 /* isTerm */, false /* isDepthFirst */);
  }

  uint32_t getAddr() {
    return addr28_ << 4;
  }

  void print() {
    printf("GenericPtr: addr 0x%x, queue %d, term %d\n", getAddr(), isQueue_, isTerm_);
  }

  operator bool() const {
    return !isTerm_;
  }
 private:
  // ORDER MATTERS!
  uint32_t isTerm_ : 1;
  uint32_t isQueue_ : 1;
  // only relevant for the link pointer inside a TrasferDescriptor.
  // In other cases, the field is reserved and should be 0
  uint32_t isDepthFirst_ : 1;
  uint32_t reserved_ : 1;
  uint32_t addr28_ : 28; // the high 28 bits of the address
};

static_assert(sizeof(GenericPtr) == 4);

class Queue {
 public:
  explicit Queue() = default;

 private:
  GenericPtr head_link_ptr_;
  GenericPtr element_link_ptr_;
};

static_assert(sizeof(Queue) == 8);

Queue globalQueue __attribute__((aligned(16)));

void UHCIDriver::setupFramePtrs() {
  assert(((uint32_t) &globalQueue & 0xF) == 0 && "Queue should be 16 bytes aligned");
  GenericPtr* framePtrList = (GenericPtr*) frame_page_addr_;
  for (int i = 0; i < 1024; ++i) {
    framePtrList[i] = GenericPtr::createQueuePtr((uint32_t) &globalQueue);
  }
}
