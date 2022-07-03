#pragma once

#include <stdint.h>
#include <kernel/idt.h>

#ifdef __cplusplus
extern "C" {
#endif

class UserProcess {
 public:
  void resume();
  void terminate();

  static UserProcess* create(uint8_t* code, uint32_t len);
  static UserProcess* load(uint8_t* elf_cont);
  static void terminate_current_process();
  static UserProcess* current();
  static void set_current(UserProcess* cur);
  /*
   * Most of the time cur is null, and we find UserProcess::current_ as the current
   * process.
   *
   * One exception is when terminating a process. We reset UserProcess::current_
   * but pass in the terminated process ptr to sched. This way, sched can give
   * more chance for processes following the terminated one to get scheduled.
   * This conform to round-robin behavior.
   */
  static void sched(UserProcess* cur = nullptr);
  static void set_frame_for_current(InterruptFrame* framePtr);

  int get_pid();
  UserProcess* clone();
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
