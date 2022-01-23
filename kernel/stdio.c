#include <stdio.h>
#include <kernel/vga.h>

int printf(const char* fmt, ...) {
  // TODO do formating
  for (const char* cp = fmt; *cp; ++cp) {
    vga_putchar(*cp);
  }
  return 0; // TODO return value!
}
