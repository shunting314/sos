/*
 * For COW fork, here are a few places that needs to be taken care to maintain
 * PhysPageStat.refcount_user properly:
 * 1. when creating a process from filesystem image (or raw binary string), the
 *    PhysPageStat.refcount_user needs to be initialized to 1 for user space pages.
 * 2. when forking a process with COW (copy on write), the PhysPageStat.refcount_user need to be incremented by one for user space pages.
 * 3. when terminating a process, PhysPageStat.refcount_user need to be decremented by one for user space pages.
 *    Free a physical page in user space only if PhysPageStat.refcount_user reaches 0 after decrementing.
 */

#include <kernel/fork.h>
#include <kernel/loader.h>
#include <kernel/user_process.h>
#include <kernel/paging.h>
#include <assert.h>

UserProcess* UserProcess::clone(bool use_cow) {
  auto* child = allocate();
  auto* parent = this;
  child->parent_pid_ = parent->get_pid();
  child->intr_frame_ = parent->intr_frame_;
  child->pgdir_ = clone_address_space(parent->pgdir_, use_cow);
  child->copy_cwd_from(parent);
  child->copy_filetab_from(parent);
  child->setup_stdio();
  return child;
}

static int dofork(bool use_cow) {
  auto* child = UserProcess::current()->clone(use_cow);
  child->intr_frame_.eax = 0; // child process return 0
  return child->get_pid();
}

int dumbfork() {
  return dofork(false);
}

int cowfork() {
  return dofork(true);
}

int spawn(const char* path, const char** argv, int fdin, int fdout) {
  int child_pid = launch(path, argv, false);

  // need set parent process id separately
  if (child_pid > 0) {
    UserProcess* child_proc = UserProcess::get_proc_by_id(child_pid);
    child_proc->set_parent_pid(UserProcess::current()->get_pid());
    child_proc->copy_cwd_from(UserProcess::current());
    if (fdin != 0) {
      child_proc->fdmov(fdin, 0);
    }
    if (fdout != 1) {
      child_proc->fdmov(fdout, 1);
    }
  }
  return child_pid;
}
