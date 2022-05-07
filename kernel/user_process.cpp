#include <kernel/user_process.h>
#include <kernel/phys_page.h>
#include <kernel/paging.h>
#include <kernel/asm_util.h>
#include <kernel/idt.h>
#include <assert.h>
#include <string.h>

// allocate 7 pages for user stack. Leave 1 page unmapped to catch stack overflow
uint32_t user_stack_start = 0x40001000;
uint32_t user_process_va_start = 0x40008000;

// only support at most this many processes for now
#define N_PROCESS 1024

UserProcess g_process_list[N_PROCESS];

UserProcess* UserProcess::current_ = nullptr;

void UserProcess::set_frame_for_current(InterruptFrame* framePtr) {
  // UserProcess::current_ can be nullptr if interrupt happens in kernel
  // mode
  if (UserProcess::current_) {
    UserProcess::current_->intr_frame_ = *framePtr;
  }
}

void UserProcess::resume() {
  current_ = this;
  asm_set_cr3(pgdir_);
  intr_frame_.returnFromInterrupt();
}

UserProcess* UserProcess::allocate() {
  for (int i = 0; i < N_PROCESS; ++i) {
    if (!g_process_list[i].allocated_) {
      g_process_list[i].allocated_ = true;
      return &g_process_list[i];
    }
  }
  assert(false && "Already created max number of processes");
  return (UserProcess*) nullptr;
}

UserProcess* UserProcess::create(uint8_t* code, uint32_t len) {
  UserProcess* proc = UserProcess::allocate();

  // allocate page dir
  uint32_t pgdir = alloc_phys_page();
  proc->pgdir_ = pgdir;
  memmove((void*) pgdir, (void*) kernel_page_dir, 4096);
  // user read/write
  map_region_alloc(pgdir, user_stack_start, 4096 * 7, MAP_FLAG_USER | MAP_FLAG_WRITE); // stack
  map_region_alloc(pgdir, user_process_va_start, len, MAP_FLAG_USER | MAP_FLAG_WRITE);
  asm_set_cr3(pgdir);
  memmove((void*) user_process_va_start, (void*) code, (int) len);

  memset(&proc->intr_frame_, 0, sizeof(proc->intr_frame_));
  proc->intr_frame_.eip = user_process_va_start;
  proc->intr_frame_.esp = user_process_va_start;
  proc->intr_frame_.cs = USER_CODE_SEG;
  proc->intr_frame_.ss = USER_DATA_SEG;
  // enable IF
  proc->intr_frame_.eflags = 0x200;

  return proc;
}

void UserProcess::sched() {
  // don't do anything if there is no current process
  if (!UserProcess::current_) {
    return;
  }
  int start_idx = (UserProcess*) UserProcess::current_ - &g_process_list[0];
  int curr_idx = start_idx;
  for (int i = 0; i < N_PROCESS; ++i) {
    curr_idx = (curr_idx + 1) % N_PROCESS;
    UserProcess* next_proc = &g_process_list[curr_idx];
    if (next_proc->allocated_) {
      next_proc->resume();
    }
  }
  // TODO: we should either always have an idle process or fall
  // to the kernel shell
  assert(false && "No active processes");
}
