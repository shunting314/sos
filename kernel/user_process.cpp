#include <kernel/user_process.h>
#include <kernel/phys_page.h>
#include <kernel/paging.h>
#include <kernel/asm_util.h>
#include <assert.h>
#include <string.h>

// allocate 7 pages for user stack. Leave 1 page unmapped to catch stack overflow
uint32_t user_stack_start = 0x40001000;
uint32_t user_process_va_start = 0x40008000;

// TODO right now only allow a single user process. Improve this soon
class UserProcess {
 public:
  // TODO: store gprs information etc for the process for we can resume it
  bool allocated_;
} g_user_process;

void create_process_and_run(uint8_t* code, uint32_t len) {
  assert(!g_user_process.allocated_);
  g_user_process.allocated_ = true;

  // allocate page dir
  uint32_t pgdir = alloc_phys_page();
  memmove((void*) pgdir, (void*) kernel_page_dir, 4096);
  uint32_t pgcode = alloc_phys_page();
  // user read/write
  map_region_alloc(pgdir, user_stack_start, 4096 * 7, MAP_FLAG_USER | MAP_FLAG_WRITE); // stack
  map_region_alloc(pgdir, user_process_va_start, len, MAP_FLAG_USER | MAP_FLAG_WRITE);
  asm_set_cr3(pgdir);
  memmove((void*) user_process_va_start, (void*) code, (int) len);

  // TODO record the UserProcess* for the current process
  asm_enter_user_mode(user_process_va_start, user_process_va_start);
}
