#include <stdint.h>
#include <assert.h>
#include <kernel/asm_util.h>
#include <stdio.h>
#include <kernel/ioport.h>
#include <kernel/keyboard.h>

#define NIDT_ENTRY 256

// NOTE: the order of fields here is reverse to the mental order:
// the field defined in upper position comes in lower address.
struct InterruptFrame {
  uint32_t all_gpr[8]; // all general purpose registers pushed by pusha
  uint32_t error_code;
  uint32_t eip;
  uint32_t padded_cs;
  uint32_t eflags;

  // these 2 fields are only available if previlege level changes
  uint32_t esp;
  uint32_t padded_ss;

  void returnFromInterrupt() {
    asm_return_from_interrupt(&all_gpr);
  }
};

static_assert(sizeof(InterruptFrame) == 24 + 32);

// the handler for interrupts we care. Force C symbol to make it convenient to call
// it from assembly.
extern "C" void interrupt_handler(int32_t intNum, InterruptFrame* framePtr) {
  if (intNum == 32) { // ignore timer
    framePtr->returnFromInterrupt();
  }
  if (intNum == 32 + 1) { // keyboard
    handleKeyboard();
    framePtr->returnFromInterrupt();
  }
  if (intNum == 48) { // do nothing for syscall for now
    printf("Received a system call\n");
    framePtr->returnFromInterrupt();
  }
  printf("Handering interrupt %d (0x%x), error code is %d (0x%x), saved eip 0x%x\n", intNum, intNum, framePtr->error_code, framePtr->error_code, framePtr->eip);
  assert(false && "Interrupt not implemented yet");
  framePtr->returnFromInterrupt();
}

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

    attr_first_byte = 0x00;
    attr_type = 0x0E;
    dpl = 0;
    present = 1;
  }

  void set_dpl(int dpl) {
    this->dpl = dpl;
  }
 private:
  uint16_t off_low;
  uint16_t segment_selector = 0x08; // kernel code segment
  // assumes DPL 0, 32 bit size
  // default member initializer for bitfield is only available in C++20. So we set the default in ctor.
  uint16_t attr_first_byte : 8 /* = 0x00 */;
  uint16_t attr_type : 5;
  uint16_t dpl : 2;
  uint16_t present : 1;
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

extern "C" void set_handlers() {
#define SET_HANDLER_FOR(no) void interrupt_entry_ ## no(); idt[no] = InterruptGateDescriptor((void*) interrupt_entry_ ## no);
  SET_HANDLER_FOR(0);
  SET_HANDLER_FOR(1);
  SET_HANDLER_FOR(2);
  SET_HANDLER_FOR(3);
  SET_HANDLER_FOR(4);
  SET_HANDLER_FOR(5);
  SET_HANDLER_FOR(6);
  SET_HANDLER_FOR(7);
  SET_HANDLER_FOR(8);
  SET_HANDLER_FOR(9);
  SET_HANDLER_FOR(10);
  SET_HANDLER_FOR(11);
  SET_HANDLER_FOR(12);
  SET_HANDLER_FOR(13);
  SET_HANDLER_FOR(14);
  SET_HANDLER_FOR(15);
  SET_HANDLER_FOR(16);
  SET_HANDLER_FOR(17);
  SET_HANDLER_FOR(18);
  SET_HANDLER_FOR(19);
  SET_HANDLER_FOR(20);
  SET_HANDLER_FOR(21);
  SET_HANDLER_FOR(22);
  SET_HANDLER_FOR(23);
  SET_HANDLER_FOR(24);
  SET_HANDLER_FOR(25);
  SET_HANDLER_FOR(26);
  SET_HANDLER_FOR(27);
  SET_HANDLER_FOR(28);
  SET_HANDLER_FOR(29);
  SET_HANDLER_FOR(30);
  SET_HANDLER_FOR(31);
  SET_HANDLER_FOR(32);
  SET_HANDLER_FOR(33);
  SET_HANDLER_FOR(34);
  SET_HANDLER_FOR(35);
  SET_HANDLER_FOR(36);
  SET_HANDLER_FOR(37);
  SET_HANDLER_FOR(38);
  SET_HANDLER_FOR(39);
  SET_HANDLER_FOR(40);
  SET_HANDLER_FOR(41);
  SET_HANDLER_FOR(42);
  SET_HANDLER_FOR(43);
  SET_HANDLER_FOR(44);
  SET_HANDLER_FOR(45);
  SET_HANDLER_FOR(46);
  SET_HANDLER_FOR(47);
  SET_HANDLER_FOR(48);
  idt[48].set_dpl(3); // allow user mode call 'int $48' for system calls
  SET_HANDLER_FOR(255);
#undef SET_HANDLER_FOR
}

void setup_one_8259(bool is_master) {
  uint16_t base_port = is_master ? 0x20 : 0xA0;
  Port8Bit a0(base_port), a1(base_port + 1);
  uint8_t vector_offset = is_master ? 0x20 : 0x28;

  // check 8259A datasheet for the initialization sequence
  a0.write(0x11); // ICW1
  a1.write(vector_offset); // ICW2: set vector offset
  a1.write(is_master ? 0x04 : 0x2); // ICW3: slave connects to master IR2
  // ICW4: pick 8086 mode;
  // also enable AEOI (automatic end of interrupt) mode so we don't need to
  // send an EOI command to PIC at the end of each IRQ.
  a1.write(0x03);

  a1.write(0x00); // OCW1: clear all masks
}

// map IRQ to interrupt numbers in the range of [0x20, 0x30)
static void remap_pic() {
  setup_one_8259(true); // setup master
  setup_one_8259(false); // setup slave
}

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
  set_handlers();
  asm_lidt();

  remap_pic();
  keyboardInit();
  asm_sti();
}
