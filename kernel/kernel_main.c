#include <kernel/vga.h>
#include <kernel/idt.h>
#include <stdio.h>

void kernel_main() {
  vga_clear();
  // show_palette();
  // show_flashing_digits();
  // show_running_curve();
  for (int i = 0; i < 100; ++i) {
    puts("Hello, World!");
  }
  setup_idt();
  asm("int $48");
  puts("Back from interrupt");

  while (1) {
  }
}
