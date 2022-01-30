#include <stdint.h>
#include <assert.h>
#include <kernel/asm_util.h>
#include <stdio.h>

#define NIDT_ENTRY 256

void noret_handler() {
  puts("interrupt not implemented!");
  while (1) {
  }
}

// assumes an interrupt gate rather than trap gate to make the exception
// handling easier since interrupt gate implies an cli before executing the
// handler.
class InterruptGateDescriptor {
 public:
  explicit InterruptGateDescriptor(void *_handler = (void *) noret_handler) {
    uint32_t handler = (uint32_t) _handler;
    off_low = (handler & 0xFFFF);
    off_high = (handler >> 16);
  }
 private:
  uint16_t off_low;
  uint16_t segment_selector = 0x08; // kernel code segment
  // assumes DPL 0, 32 bit size
  uint16_t attr = 0x8F00;
  uint16_t off_high;
} __attribute__((packed));

static_assert(sizeof(InterruptGateDescriptor) == 8);

extern "C" {
InterruptGateDescriptor idt[NIDT_ENTRY];
}

struct IDTRDescriptor {
  IDTRDescriptor() : limit_(sizeof(idt) - 1), idt_((uint32_t) idt) {
  }

  uint16_t limit_;
  uint32_t idt_; 
} __attribute__((packed));

static_assert(sizeof(IDTRDescriptor) == 6);

IDTRDescriptor idtrdesc;

static_assert(sizeof(IDTRDescriptor) == 6);

extern "C" void setup_idt() {
  // the following assignments are needed in a very interesting reason..
  // When we load the kernel, we didn't execute the init functions.
  // So global variables are not properly constructed!
  // Don't use global objects that does meaningful things in ctor if possible.
  // Or enhance the loader to run the init functions.
  for (int i = 0; i < NIDT_ENTRY; ++i) {
    idt[i] = InterruptGateDescriptor();
  }
  idtrdesc = IDTRDescriptor();

  asm_lidt();
}
