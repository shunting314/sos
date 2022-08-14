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
#include <kernel/user_process.h>
#include <kernel/paging.h>
#include <assert.h>

// TODO implement COW
UserProcess* UserProcess::clone(bool use_cow) {
  auto* child = allocate();
  auto* parent = this;
  child->intr_frame_ = parent->intr_frame_;
  child->pgdir_ = clone_address_space(parent->pgdir_, use_cow);
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
