#include <kernel/fork.h>
#include <kernel/user_process.h>
#include <kernel/paging.h>
#include <assert.h>

// TODO implement COW
UserProcess* UserProcess::clone() {
  auto* child = allocate();
  auto* parent = this;
  child->intr_frame_ = parent->intr_frame_;
  child->pgdir_ = clone_address_space(parent->pgdir_);
  return child;
}

int dumbfork() {
  auto* child = UserProcess::current()->clone();
  child->intr_frame_.eax = 0; // child process return 0
  return child->get_pid();
}
