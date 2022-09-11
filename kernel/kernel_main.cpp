#include <kernel/vga.h>
#include <kernel/idt.h>
#include <kernel/paging.h>
#include <kernel/phys_page.h>
#include <kernel/tss.h>
#include <kernel/kshell.h>
#include <kernel/simfs.h>
#include <kernel/pci.h>
#include <kernel/nic/nic.h>
#include <kernel/init.h>
#include <kernel/sleep.h>
#include <kernel/pit.h>
#include <kernel/usb/usb.h>
#include <stdio.h>

#ifdef TEST_LARGE_KERNEL
char buf[65546] = {1};
#endif

extern "C" void kernel_main() {
  vga_clear();
  kernel_elf_init();
  puts("Hello, World!");

  init_pit();
  setup_idt();

  setup_phys_page_freelist();
  setup_paging();
  setup_tss();
  SimFs::get().init();

  lspci();
  collect_pci_devices();
  nic_init();

#ifdef TEST_SLEEP
  for (int i = 0; i < 300; ++i) {
    printf("message %d\n", i);
    msleep(1000);
  }
#endif
  usb_init();
  kshell();
}
