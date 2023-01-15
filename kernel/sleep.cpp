#include <kernel/sleep.h>
#include <kernel/idt.h>
#include <assert.h>

/*
 * A dumb sleep. Assumes the loop is not optimized away by the compiler.
 */
void dumbsleep(int niter) {
  for (int i = 0; i < niter; ++i) {
    for (int j = 0; j < 1000000; ++j) {
    }
  }
}

/*
 * Execute hlt or relying on tick is dangerous. We use InterruptGate to setup IDT.
 * So interrupts are disabled when interruptes/exceptions happens. If the open/read  * system call is triggered by user code and the service routine for these system
 * calls need to call msleep (e.g., used somewhere when accessing USB Drive), we
 * stuck in the 'hlt' instruction!
 *
 * Even if we remove the hlt instruction, we still stuck since nobody update the
 * tick when the timer interrupt is disabled.
 *
 * Ultimately to make msleep work when entering kernel from user space, we should
 * not use InterruptGate but keep interrupt enabled when entering kernel. But that's
 * complex and we can consider this in the future.
 */
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
