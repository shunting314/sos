#ifndef KERNEL_IO_H
#define KERNEL_IO_H

#include <stdint.h>
#include <assert.h>
#include <kernel/asm_util.h>

class Port8Bit {
 public:
  explicit Port8Bit(uint16_t port) : port_(port) { }

  uint8_t read() {
    uint8_t ret;
    asm(
        "inb %1, %0" : "=a"(ret) : "d"(port_) /* port should go to edx */:
    );
    return ret;
  }

  void write(uint8_t data) {
    asm volatile(
        "outb %1, %0" : : "d"(port_), "a"(data)
    );
  }
 private:
  uint16_t port_;
};

class Port16Bit {
 public:
  explicit Port16Bit(uint16_t port) : port_(port) { }

  uint16_t read() {
    uint16_t ret;
    asm(
        "inw %1, %0" : "=a"(ret) : "d"(port_) /* port should go to edx */:
    );
    return ret;
  }

  void write(uint16_t data) {
    /*
     * Add volatile keyword so compiler does not drop this asm block which
     * has side effect.
     */
    asm volatile(
        "outw %1, %0" : : "d"(port_), "a"(data)
    );
  }
 private:
  uint16_t port_;
};

class Port32Bit {
 public:
  explicit Port32Bit(uint16_t port) : port_(port) { }

  uint32_t read() {
    uint32_t ret;
    asm(
        "inl %1, %0" : "=a"(ret) : "d"(port_) /* port should go to edx */:
    );
    return ret;
  }

  void write(uint32_t data) {
    /*
     * Add volatile keyword so compiler does not drop this asm block which
     * has side effect.
     */
    asm volatile(
        "outl %1, %0" : : "d"(port_), "a"(data)
    );
  }
 private:
  uint16_t port_;
};

#endif
