#include <kernel/pic.h>
#include <kernel/ioport.h>

Port8Bit master_a0(0x20), master_a1(0x21);
Port8Bit slave_a0(0xA0), slave_a1(0xA1);

#define PIC_EOI 0x20

// explicit EOI will implicitly ack the interrupt
void pic_send_eoi(int irq) {
  if (irq >= 8) {
    // write to slave
    slave_a0.write(PIC_EOI);
  }
  // write to master
  master_a0.write(PIC_EOI);
}

static void setup_one_8259(bool is_master, Port8Bit &a0, Port8Bit &a1) {
  uint8_t vector_offset = is_master ? 0x20 : 0x28;

  // check 8259A datasheet for the initialization sequence
  a0.write(0x11); // ICW1
  a1.write(vector_offset); // ICW2: set vector offset
  a1.write(is_master ? 0x04 : 0x2); // ICW3: slave connects to master IR2
  // ICW4: pick 8086 mode;
  // also enable AEOI (automatic end of interrupt) mode so we don't need to
  // send an EOI command to PIC at the end of each IRQ.
  //
  // NOTE: even if we enable AEOI, we still need an explicit EOI sometimes
  // Check the comment in kernel/idt.cpp
  a1.write(0x03);

  a1.write(0x00); // OCW1: clear all masks
}

// map IRQ to interrupt numbers in the range of [0x20, 0x30)
void pic_remap() {
  setup_one_8259(true, master_a0, master_a1); // setup master
  setup_one_8259(false, slave_a0, slave_a1); // setup slave
}
