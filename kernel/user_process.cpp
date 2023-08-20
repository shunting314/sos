#include <kernel/user_process.h>
#include <kernel/phys_page.h>
#include <kernel/paging.h>
#include <kernel/asm_util.h>
#include <kernel/kshell.h>
#include <kernel/idt.h>
#include <kernel/file_desc.h>
#include <assert.h>
#include <string.h>

// allocate 7 pages for user stack. Leave 1 page unmapped to catch stack overflow
uint32_t user_stack_start = 0x40001000;
uint32_t user_process_va_start = 0x40008000;

// only support at most this many processes for now
#define N_PROCESS 1024

UserProcess g_process_list[N_PROCESS];

UserProcess* UserProcess::current_ = nullptr;

UserProcess* UserProcess::get_proc_by_id(int pid) {
  assert(pid >= 0 && pid < N_PROCESS);
  return g_process_list + pid;
}

int UserProcess::get_pid() {
  return this - g_process_list;
}

UserProcess* UserProcess::current() {
  return current_;
}

void UserProcess::set_current(UserProcess* cur) {
  current_ = cur;
}

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
  // let's skip process 0 for now so process id start from 1
  // this is to make sure the child process id is non-zero for fork.
  for (int i = /* 0 */ 1; i < N_PROCESS; ++i) {
    if (!g_process_list[i].allocated_) {
      g_process_list[i].allocated_ = true;
      g_process_list[i].parent_pid_ = -1;
      g_process_list[i].terminated_ = false;

      g_process_list[i].wait_for_child_ = nullptr;
      g_process_list[i].wait_for_pstatus_ = nullptr;
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

void UserProcess::sched(UserProcess* cur) {
  if (!cur) {
    cur = UserProcess::current_;
  } else {
    assert(!UserProcess::current_ && "We are terminating the current process and UserProcess::current_ should be nullptr");
  }
  // don't do anything if there is no current process
  if (!cur) {
    return;
  }
  int start_idx = (UserProcess*) cur - &g_process_list[0];
  int curr_idx = start_idx;
  int nallocated = 0;
  for (int i = 0; i < N_PROCESS; ++i) {
    curr_idx = (curr_idx + 1) % N_PROCESS;
    UserProcess* next_proc = &g_process_list[curr_idx];

    if (!next_proc->allocated_) {
      continue;
    }
    ++nallocated;
    if (next_proc->wait_for_child_) {
      next_proc->waitchild(); 

      // reach here if next_proc is not ready for running yet. Check the next one.
      continue;
    } else {
      next_proc->resume();
    }
  }

  // no active processes any more, run kshell
  assert(nallocated == 0 && "deadlock?");
  kshell();
}

// file descriptor related APIs. Move to a standalone file if the size for these
// APIs grow

int UserProcess::releaseFd(int fd) {
  if (fd < 0 || fd >= MAX_OPEN_FILE) {
    return -1;
  }
	FileDesc* fdptr = filetab_[fd];
	if (!fdptr) {
		return 0;
	}
	filetab_[fd] = nullptr;
	decref_file_desc(fdptr);
	return 1;
}

// TODO: we should cache DirEnt to avoid traverse the path everytime when accessing the
// file.
int UserProcess::allocFd(const char* path, int rwflags, bool checkall) {
	int fd = -1;
	int start_idx = checkall ? 0 : 3;
	for (fd = start_idx; fd < MAX_OPEN_FILE && filetab_[fd]; ++fd) {
	}
	if (fd == MAX_OPEN_FILE) {
		return -1; // too many open file
	}
	FileDesc* fdptr = alloc_file_desc();
	if (!fdptr) {
		return -1; // out of file desc
	}
	filetab_[fd] = fdptr;

	if (fdptr->init(path, rwflags) < 0) {
		// we should release the allocated resource if failure happens
		releaseFd(fd);
		return -1;
	} else {
		return fd;
	}
}

int UserProcess::waitpid(int child_pid, int *pstatus) {
  if (child_pid < 0 || child_pid >= N_PROCESS) {
    return -1;
  }
  UserProcess* child_process = &g_process_list[child_pid];

  // only the parent process can wait for the child and query its exit status
  if (child_process->parent_pid_ != get_pid()) {
    return -1;
  }
  assert(child_process->allocated_);
  if (child_process->terminated_) {
    // this function is called by syscall. So we still have access to *pstatus
    // using the current active page directory.
    *pstatus = child_process->exit_status_;
    child_process->release();
    return child_pid;
  } else {
    // the child process has not exit yet.
    // Record the child process and pstatus pointer so the scheduler can
    // handle them later when the child process has terminated.

    wait_for_child_ = child_process;
    wait_for_pstatus_ = pstatus;
    sched(nullptr);
    // never return here
  }
  assert(false && "never reach here");
  return -1;
}

/*
 * Resume the current process if child has already terminated. Otherwise return.
 * Called by the scheduler.
 */
void UserProcess::waitchild() {
  assert(wait_for_child_);
  assert(wait_for_pstatus_);

  assert(wait_for_child_->allocated_);
  if (!wait_for_child_->terminated_) {
    return;  // child is still alive
  }

  int *user_mode_pstatus = wait_for_pstatus_;
  UserProcess* child_process = wait_for_child_;

  // clear wait_for_pstatus_ and wait_for_child_ so the scheduler will schedule
  // this process next time.
  wait_for_child_ = nullptr;
  wait_for_pstatus_ = nullptr;

  int child_status = child_process->exit_status_;
  child_process->release(); // release the child process data structure

  // setup the return value properly and resume the current process
  intr_frame_.eax = child_process->get_pid();

  // we need reload the current process's cr3 before we can write *user_mode_pstatus
  asm_set_cr3(pgdir_);
  *user_mode_pstatus = child_status;
  resume();
}
