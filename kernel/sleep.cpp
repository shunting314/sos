#include <kernel/sleep.h>
#include <kernel/idt.h>
#include <assert.h>

void msleep(int nms) {
  int64_t start_tick = getTick();
  // 1 tick == 10 ms
  // add 1 more tick to be conservative
  //
  // The BIOS set the frequency so a tick is equivalent to 55 ms. We change it
  // in init_pit() so each tick is equivalent to 10 ms.
  static int tick_weight = 10;
  int64_t end_tick = start_tick + (nms + tick_weight - 1) / tick_weight + 1;
  while (getTick() < end_tick) {
    asm("hlt");
  }
}
