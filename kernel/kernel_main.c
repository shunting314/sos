#include <kernel/vga.h>
#include <kernel/idt.h>
#include <kernel/keyboard.h>
#include <kernel/paging.h>
#include <kernel/phys_page.h>
#include <kernel/user_process.h>
#include <kernel/tss.h>
#include <stdio.h>

void launch_manual_user_process() {
  /*
   * nop
   * int $0x30
   * 1: jmp 1b
   */
  uint8_t code[] = {
    0x90,
    0xcd, 0x30,
    0xeb, 0xfe,
  };
  create_process_and_run(code, sizeof(code) / sizeof(code[0]));
}

void kernel_main() {
  vga_clear();
  // show_palette();
  // show_flashing_digits();
  // show_running_curve();
  puts("Hello, World!");
  setup_idt();
  asm("int $48");
  puts("Back from interrupt");

  setup_phys_page_freelist();
  setup_paging();
  setup_tss();

  // TODO support consuming a list of commands in kernel mode
  launch_manual_user_process();

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
