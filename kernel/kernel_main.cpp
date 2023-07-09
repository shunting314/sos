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

void test_kernel();

#ifdef TEST_LARGE_KERNEL
char buf[65546] = {1};
#endif

#define TEST_BACKTRACE 1

#if TEST_BACKTRACE
void g() {
  backtrace();
}

void f() {
  g();
}

void test_backtrace() {
  f();
}
#endif

void setup_malloc(void *start, uint32_t size); // this is defined in clib/malloc.cpp
static void setup_kernel_malloc() {
  // be consistent with setup_paging()
  setup_malloc((void*) phys_mem_amount, (1 << 20) * 2); 
}

extern "C" void kernel_main() {
  vga_clear();
  kernel_elf_init();
  puts("Hello, World!");

  init_pit();
  setup_idt();

  setup_phys_page_freelist();
  setup_paging();
  setup_kernel_malloc();
  setup_tss();

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
  SimFs::get().init();

  // NOTE the allocated memory from readFile is not freed.
  set_dwarf_ctx_elf_buf(SimFs::get().readFile("/kernel.sym"));

  test_kernel();
#if TEST_BACKTRACE
  test_backtrace();
#endif
  kshell();
}
