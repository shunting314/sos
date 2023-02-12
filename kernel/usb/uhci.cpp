#include <kernel/usb/uhci.h>
#include <kernel/usb/usb_proto.h>
#include <kernel/usb/usb_device.h>

#define PID_SETUP 0x2D
#define PID_IN 0x69
#define PID_OUT 0xE1

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

  uint32_t getAddr() const {
    return addr28_ << 4;
  }

  void print() const {
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

class TransferDescriptor {
 public:
  explicit TransferDescriptor() = default;
  explicit TransferDescriptor(
    TransferDescriptor* next,
    uint8_t pid,
    uint8_t device_address,
    uint8_t endpoint,
    uint8_t data_toggle,
    uint32_t max_len_off1,
    uint32_t buffer_pointer
  ) : act_len_off1_(0),
      reserved_0(0),
      status_(0x80), // active
      ioc_(0),
      iso_(0),
      ls_(0),
      cerr_(1), // no retry
      spd_(0),
      reserved_1(0),
      reserved_2(0) {
    if (next) {
      link_pointer_ = GenericPtr((uint32_t) next, false, false, true);
    } else {
      link_pointer_ = GenericPtr(0, false, true);
    }
    pid_ = pid;
    device_address_ = device_address;
    endpoint_ = endpoint;
    data_toggle_ = data_toggle;
    max_len_off1_ = max_len_off1;
    buffer_pointer_ = buffer_pointer;
  }

  void print() const {
    uint32_t* arr = (uint32_t*) this;
    printf("Transmit descriptor:\n");
    link_pointer_.print();
    printf("  STATUS 0x%x, max_len_off1 %d\n", status_, max_len_off1_);
    for (int i = 0; i < 4; ++i) {
      printf("  0x%x\n", arr[i]);
    }
  }

  bool isActive() {
    return status_ & 0x80;
  }

  uint8_t getStatus() {
    return status_;
  }
 private:
  // dword0
  GenericPtr link_pointer_;

  // dword1
  uint32_t act_len_off1_ : 11;
  uint32_t reserved_0 : 5;
  uint32_t status_ : 8;
  uint32_t ioc_ : 1;
  uint32_t iso_ : 1;
  uint32_t ls_ : 1;
  uint32_t cerr_ : 2;
  uint32_t spd_ : 1;
  uint32_t reserved_1 : 2;

  // dword2
  uint32_t pid_ : 8;
  uint32_t device_address_ : 7;
  uint32_t endpoint_ : 4;
  uint32_t data_toggle_ : 1;
  uint32_t reserved_2 : 1;
  uint32_t max_len_off1_ : 11;

  // dword3
  uint32_t buffer_pointer_;

  uint32_t dummy[4] = { 0};
};

static_assert(sizeof(TransferDescriptor) == 32);

TransferDescriptor tdpool[512] __attribute__((aligned(16)));

// simply check the pool has enough TDs. There may not be
// concurrent/overlap reservatoins. The next reserve call
// automatically frees the TDs obtains by the previous call.
TransferDescriptor* reserveTDs(int num) {
  static int tot = sizeof(tdpool) / sizeof(*tdpool);
  assert(num > 0);
  assert(num <= tot);
  return tdpool;
}

class Queue {
 public:
  explicit Queue() = default;

  void setElementLinkPtr(TransferDescriptor* td) {
    element_link_ptr_ = GenericPtr((uint32_t) td, 0, 0);
  }

  GenericPtr getElementLinkPtr() {
    return element_link_ptr_;
  }

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

void UHCIDriver::sendDeviceRequest(USBDevice<UHCIDriver>* device, DeviceRequest* req, void *buf) {
  int maxPacketSize = device->getMaxPacketSize();
  int maxLength = req->wLength;
  assert((maxLength > 0 && buf) || (maxLength == 0 && !buf));
  uint8_t reqType = req->bmRequestType;

  // need 2 more for the SETUP and STATUS packets
  int ntd = (maxLength + maxPacketSize - 1) / maxPacketSize + 2;
  TransferDescriptor* tds = reserveTDs(ntd);
  TransferDescriptor* reqTD = &tds[0];

  *reqTD = TransferDescriptor(
    &tds[1],
    PID_SETUP,
    device->getAddr(),
    0 /* endpoint */,
    0 /* data_toggle */,
    sizeof(*req) - 1, /* max_len_off1 */
    (uint32_t) req /* buffer-pointer */
  );

  // payloads
  uint32_t off = 0, len;
  uint8_t payload_pid = (reqType & 0x80) ? PID_IN : PID_OUT;
  for (int i = 1; i < ntd - 1; ++i) {
    TransferDescriptor* cur = &tds[i];
    int len = min(maxPacketSize, maxLength - off);
    assert(len > 0);
    *cur = TransferDescriptor(
      cur + 1,
      payload_pid,
      device->getAddr(),
      0 /* endpoint */,
      (i & 1) /* data toggle */,
      len - 1 /* max_len_off1 */,
      (uint32_t) buf + off
    );
    off += len;
  }

  // status
  TransferDescriptor *statusTD = &tds[ntd - 1];
  uint8_t status_pid = (reqType & 0x80) ? PID_OUT : PID_IN;
  *statusTD = TransferDescriptor(
    nullptr,
    status_pid,
    device->getAddr() /* device_address */,
    0 /* endpoint */,
    1 /* data_toggle */,
    0x7FF /* max_len_off1 */,
    0 /* buffer_pointer */
  );
  globalQueue.setElementLinkPtr(reqTD); // start processing
  while (statusTD->isActive()) {
    msleep(1);
  }
  assert(!globalQueue.getElementLinkPtr());
}

void UHCIDriver::bulkSend(USBDevice<UHCIDriver>* device, const EndpointDescriptor& desc, uint32_t& toggle, const uint8_t* buf, int bufsize) {
  int maxPacketSize = desc.wMaxPacketSize;
  int ntd = (bufsize + maxPacketSize - 1) / maxPacketSize;
  TransferDescriptor* tds = reserveTDs(ntd);
  assert(bufsize > 0);
  int off = 0;
  for (int i = 0; i < ntd; ++i) {
    TransferDescriptor* cur = &tds[i];
    TransferDescriptor* next = (i == ntd - 1 ? nullptr : &tds[i + 1]);

    int cursize = min(maxPacketSize, bufsize - off);
    *cur = TransferDescriptor(
      next,
      PID_OUT,
      device->getAddr(),
      desc.bEndpointAddress,
      (toggle++) % 2,
      cursize - 1, // NOTE: need off by 1!
      (uint32_t) buf + off
    );
    off += cursize;
  }
  assert(off == bufsize);
  globalQueue.setElementLinkPtr(tds);
  while (tds[ntd - 1].isActive()) {
    msleep(1);
  }
  assert(!globalQueue.getElementLinkPtr());
  for (int i = 0; i < ntd; ++i) {
    assert(tds[i].getStatus() == 0);
  }
}

void UHCIDriver::bulkRecv(USBDevice<UHCIDriver>* device, const EndpointDescriptor& desc, uint32_t& toggle, uint8_t* buf, int bufsize) {
  int maxPacketSize = desc.wMaxPacketSize;
  int ntd = (bufsize + maxPacketSize - 1) / maxPacketSize;
  TransferDescriptor* tds = reserveTDs(ntd);
  assert(bufsize > 0);

  int off = 0;
  for (int i = 0; i < ntd; ++i) {
    TransferDescriptor* cur = &tds[i];
    TransferDescriptor* next = (i == ntd - 1 ? nullptr : &tds[i + 1]);

    int cursize = min(maxPacketSize, bufsize - off);
    assert(desc.bEndpointAddress & 0x80);
    *cur = TransferDescriptor(
      next,
      PID_IN,
      device->getAddr(),
      desc.bEndpointAddress,
      (toggle++) % 2,
      cursize - 1, // NOTE: need off by 1!
      (uint32_t) buf + off
    );
    off += cursize;
  }
  assert(off == bufsize);
  globalQueue.setElementLinkPtr(tds);
  while (tds[ntd - 1].isActive()) {
    msleep(1);
  }
  assert(!globalQueue.getElementLinkPtr());
  for (int i = 0; i < ntd; ++i) {
    assert(tds[i].getStatus() == 0);
  }
}
