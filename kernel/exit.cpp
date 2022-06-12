#include <kernel/user_process.h>
#include <kernel/paging.h>
#include <assert.h>
#include <string.h>

void UserProcess::terminate_current_process() {
  UserProcess::current()->terminate();
}

void UserProcess::terminate() {
  // switch to the kernel pagedir
  asm_set_cr3((uint32_t) kernel_page_dir);

  release_pgdir(pgdir_);

  // release the process struture and clear the state
  memset(this, 0, sizeof(*this));
  set_current(nullptr); // reset current process ptr

  sched(this);
  assert(false && "can not reach here");
}
