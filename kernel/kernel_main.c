#include <kernel/vga.h>

void kernel_main() {
  vga_clear();
  // show_palette();
  // show_flashing_digits();
  show_running_curve();
  while (1) {
  }
}
