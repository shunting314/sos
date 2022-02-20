#include <kernel/vga.h>
#include <kernel/idt.h>
#include <kernel/keyboard.h>
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

  char line[1024];
  while (1) {
    printf("> ");
    int got = keyboardReadLine(line, sizeof(line));
    if (got > 0 && line[got - 1] == '\n') {
      line[got - 1] = '\0';
    }
    printf("Got: %s\n", line);
  }
}
