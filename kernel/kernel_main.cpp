#include <kernel/vga.h>
#include <kernel/idt.h>
#include <kernel/paging.h>
#include <kernel/phys_page.h>
#include <kernel/tss.h>
#include <kernel/kshell.h>
#include <kernel/simfs.h>
#include <kernel/pci.h>
#include <kernel/nic/nic.h>
#include <stdio.h>

extern "C" void kernel_main() {
  vga_clear();
  puts("Hello, World!");
  setup_idt();

  setup_phys_page_freelist();
  setup_paging();
  setup_tss();
  SimFs::get().init();

  lspci();
  collect_pci_devices();
  nic_init();
  kshell();
}
