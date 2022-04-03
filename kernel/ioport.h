#ifndef KERNEL_IO_H
#define KERNEL_IO_H

#include <stdint.h>
#include <assert.h>
#include <kernel/asm_util.h>

class Port8Bit {
 public:
  explicit Port8Bit(uint16_t port) : port_(port) { }

  uint8_t read() {
    return asm_inb(port_);
  }

  void write(uint8_t data) {
    // TODO: use inline assembly
    asm_outb(port_, data);
  }
 private:
  uint16_t port_;
};

class Port16Bit {
 public:
  explicit Port16Bit(uint16_t port) : port_(port) { }

  uint16_t read() {
    return asm_inw(port_);
  }

  void write(uint16_t data) {
    return asm_outw(port_, data);
  }
 private:
  uint16_t port_;
};

#endif
