#include <kernel/vga.h>
#include <kernel/idt.h>
#include <kernel/keyboard.h>
#include <kernel/paging.h>
#include <kernel/phys_page.h>
#include <kernel/user_process.h>
#include <kernel/tss.h>
#include <stdio.h>

UserProcess *create_process_printloop(char ch) {
  /*
   * nop
   * mov $1, %eax # syscall number 1 for write
   * mov $1, %ebx # file descriptor
   * mov $addr, %ecx # buf 0x40008100
   * mov $1, %edx # cnt
   * mov $0, %esi
   * mov $0, %edi
   * int $0x30
   * 1: jmp 1b
   */
  uint8_t code[512] = {
    0x90, // addr 0x40008000
    0xb8, 0x01, 0x00, 0x00, 0x00,
    0xbb, 0x01, 0x00, 0x00, 0x00,
    0xb9, 0x00, 0x81, 0x00, 0x40,
    0xba, 0x01, 0x00, 0x00, 0x00,
    0xbe, 0x00, 0x00, 0x00, 0x00,
    0xbf, 0x00, 0x00, 0x00, 0x00,
    0xcd, 0x30,
    0xeb, 0xde,
    0xeb, 0xfe,
  };
  code[256] = ch; // data
  return UserProcess::create(code, sizeof(code) / sizeof(code[0]));
}

extern "C" void kernel_main() {
  vga_clear();
  // show_palette();
  // show_flashing_digits();
  // show_running_curve();
  puts("Hello, World!");
  setup_idt();
  // asm("int $48");
  // puts("Back from interrupt");

  setup_phys_page_freelist();
  setup_paging();
  setup_tss();

  // TODO support consuming a list of commands in kernel mode
  printf("Create process A\n");
  UserProcess *proc_A = create_process_printloop('A');
  printf("Create process B\n");
  UserProcess *proc_B = create_process_printloop('B');
  proc_A->resume();

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
