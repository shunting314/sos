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
  static UserProcess* load(uint8_t* elf_cont);
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

extern uint32_t user_stack_start;
extern uint32_t user_process_va_start;

#ifdef __cplusplus
}
#endif
