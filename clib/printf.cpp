#include <stdarg.h>

typedef void putchar_fn_t(char ch);

void printnum(int num, int base, putchar_fn_t* putchar_fn) {
  static char digits[] = "0123456789abcdef";
  // handle negative number
  if (num < 0) {
    putchar_fn('-');
    printnum(-num, base, putchar_fn);
    return;
  }
  // handle single digit
  if (num >= 0 && num < base) {
    putchar_fn(digits[num]);
    return;
  }
  // handle multi digit
  printnum(num / base, base, putchar_fn);
  putchar_fn(digits[num % base]);
}

// internal implementation of vprintf
int vprintf_int(const char*fmt, va_list va, putchar_fn_t* putchar_fn) {
  int percent_mode = 0;
  for (const char* cp = fmt; *cp; ++cp) {
    char ch = *cp;
    if (ch == '%') {
      if (percent_mode) {
        // in "%%" the first percent escape the second one and result in a literal '%'
        putchar_fn(ch);
      }
      percent_mode = !percent_mode;
      continue;
    }

    if (percent_mode) {
      // TODO support other formattings
      switch (ch) {
      case 'd': {
        int num = va_arg(va, int);
        printnum(num, 10, putchar_fn);
        percent_mode = 0;
        goto next;
      }
      case 'x': {
        int num = va_arg(va, int);
        printnum(num, 16, putchar_fn);
        percent_mode = 0;
        goto next;
      }
      case 's': {
        const char *str = va_arg(va, const char*);
        while (*str) {
          putchar_fn(*str++);
        }
        percent_mode = 0;
        goto next;
      }
      case 'c': {
        char arg_ch = va_arg(va, char);
        putchar_fn(arg_ch);
        percent_mode = 0;
        goto next;
      }
      default:
        break;
      }
    }

    putchar_fn(ch);
next:;
  }
  // should not be in percent mode here
  return 0; // TODO return value!
}
