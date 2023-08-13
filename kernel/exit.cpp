#include <kernel/user_process.h>
#include <kernel/paging.h>
#include <assert.h>
#include <string.h>

void UserProcess::terminate_current_process(int status) {
  UserProcess::current()->terminate(status);
}

void UserProcess::terminate(int status) {
  // switch to the kernel pagedir
  asm_set_cr3((uint32_t) kernel_page_dir);

  release_pgdir(pgdir_);

  // release the process struture and clear the state
  if (parent_pid_ < 0) {
    release(); // release directly since there would be no parent process waiting for this one
  } else {
    // turns into a zombie process
    terminated_ = true;
    exit_status_ = status;
  }
  set_current(nullptr); // reset current process ptr

  sched(this);
  assert(false && "can not reach here");
}
