#include <stdio.h>
#include <stdarg.h>
#include <kernel/vga.h>

void printnum(int num, int base) {
  static char digits[] = "0123456789abcdef";
  // handle negative number
  if (num < 0) {
    vga_putchar('-');
    printnum(-num, base);
  }
  // handle single digit
  if (num >= 0 && num < base) {
    vga_putchar(digits[num]);
    return;
  }
  // handle multi digit
  printnum(num / base, base);
  vga_putchar(digits[num % base]);
}

int printf(const char* fmt, ...) {
  int percent_mode = 0;
  va_list va;
  va_start(va, fmt);
  for (const char* cp = fmt; *cp; ++cp) {
    char ch = *cp;
    if (ch == '%') {
      if (percent_mode) {
        // in "%%" the first percent escape the second one and result in a literal '%'
        vga_putchar(ch);
      }
      percent_mode = !percent_mode;
      continue;
    }

    if (percent_mode) {
      // TODO support other formattings
      switch (ch) {
      case 'd': {
        int num = va_arg(va, int);
        printnum(num, 10);
        percent_mode = 0;
        goto next;
      }
      case 'x': {
        int num = va_arg(va, int);
        printnum(num, 16);
        percent_mode = 0;
        goto next;
      }
      case 's': {
        const char *str = va_arg(va, const char*);
        while (*str) {
          vga_putchar(*str++);
        }
        percent_mode = 0;
        goto next;
      }
      default:
        break;
      }
    }

    vga_putchar(ch);
next:;
  }
  // should not be in percent mode here
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
