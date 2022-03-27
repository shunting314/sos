#pragma once

#include <stdint.h>
#include <kernel/idt.h>

#ifdef __cplusplus
extern "C" {
#endif

class UserProcess {
 public:
  void resume();
  static UserProcess* create(uint8_t* code, uint32_t len);
  static void sched();
  static void set_frame_for_current(InterruptFrame* framePtr);
 private:
  static UserProcess* allocate();
  static UserProcess* current_;
 public:
  bool allocated_;
  uint32_t pgdir_;
  InterruptFrame intr_frame_;
};

#ifdef __cplusplus
}
#endif
