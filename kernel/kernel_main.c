#include <kernel/vga.h>
#include <stdio.h>

void kernel_main() {
  vga_clear();
  // show_palette();
  // show_flashing_digits();
  // show_running_curve();
  for (int i = 0; i < 100; ++i) {
    printf("Hello, World!\n");
  }
  printf("abc\n");
  printf("def\n");
  while (1) {
  }
}
