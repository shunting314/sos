#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <kernel/vga.h>

// defined in common/printf.cpp
typedef void putchar_fn_t(char ch);
typedef void change_color_fn_t(int color_code);
int vprintf_int(const char*fmt, va_list va, putchar_fn_t* putchar_fn, change_color_fn_t* change_color_fn);

int printf(const char* fmt, ...) {
  // sometimes we want print to be slower to ease debugging and observing
#ifdef INJECT_PRINT_DELAY
  int busy_loop_cnt = INJECT_PRINT_DELAY;

  // -DINJECT_PRINT_DELAY implies that INJECT_PRINT_DELAY == 1
  if (busy_loop_cnt == 0 || busy_loop_cnt == 1) {
    busy_loop_cnt = 20000000;
  }
  for (int i = 0; i < busy_loop_cnt; ++i) {
  }
#endif
  va_list va;
  va_start(va, fmt);

  int out = vprintf_int(fmt, va, vga_putchar, change_global_color);
  return out;
}

int puts(const char *s) {
  int i;
  for (i = 0; s[i]; ++i) {
    vga_putchar(s[i]);
  }
  vga_putchar('\n');
  return i;
}

int putchar(int ch) {
  vga_putchar((char) ch);
  return ch;
}
