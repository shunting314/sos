#include <stdio.h>
#include <kernel/vga.h>

int printf(const char* fmt, ...) {
  // TODO do formating
  for (const char* cp = fmt; *cp; ++cp) {
    vga_putchar(*cp);
  }
  return 0; // TODO return value!
}

int puts(const char *s) {
  int i;
  for (i = 0; s[i]; ++i) {
    vga_putchar(s[i]);
  }
  vga_putchar('\n');
  return i;
}
