#pragma once

#include <stdint.h>
#include <kernel/idt.h>
#include <kernel/file_desc.h>
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_OPEN_FILE 64

class UserProcess {
 public:
  void resume();
  void terminate(int status);

  static UserProcess* get_proc_by_id(int pid);
  static UserProcess* create(uint8_t* code, uint32_t len);
  static UserProcess* load(uint8_t* elf_cont, const char** argv);
  static void terminate_current_process(int status);
  // call by syscall
  int waitpid(int child_pid, int *pstatus);
  // call by the scheduler. Schedule the current if child has existed.
  // return otherwise.
  void waitchild();
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
  UserProcess* clone(bool use_cow);
  uint32_t getPgdir() { return pgdir_; }

  void set_parent_pid(int ppid) { parent_pid_ = ppid; }

  // The API does the following things:
  // 1. Find a free filetab_ slot
  // 2. assign an available FileDesc to it
  // 3. setup the FileDesc using path and rwflags
  // 4. return the index to the filetab_.
  //
  // If checkall is true, the API will also check slot 0/1/2 (stdin/stdout,stderr)
  // for available slots.
  int allocFd(const char* path, int rwflags, bool checkall=false);

  // return true if the fd was used previously
  int releaseFd(int fd);
  // release all fd that are still open
  void releaseAllFds();
  FileDescBase* getFdptr(int fd) {
    if (!filetab_[fd]) {
      printf("Missing fd %d\n", fd);
    }
    assert(filetab_[fd]);
    return filetab_[fd];
  }

  void copy_cwd_from(UserProcess* other) {
    // 'other' most likely is the parent process.
    assert(cwd_);
    assert(other->cwd_);
    free(cwd_);
    cwd_ = strdup(other->cwd_);
  }

  void copy_filetab_from(UserProcess* other) {
    for (int i = 0; i < MAX_OPEN_FILE; ++i) {
      auto* fd = other->filetab_[i];
      if (fd) {
        ++fd->refcount_;
        filetab_[i] = fd;
      }
    }
  }

  void setup_stdio() {
    if (!filetab_[0]) {
      filetab_[0] = acquire_console_file_desc();
    }
    if (!filetab_[1]) {
      filetab_[1] = acquire_console_file_desc();
    }
  }

  const char* getCwd() const {
    return cwd_;
  }
  /*
   * Consider path as a relative path to cwd_ if it's not started with
   * '/'.
   *
   * TODO: Don't support '.', '..' yet.
   */
  int chdir(const char* path);

 private:
  static UserProcess* allocate();
  void release() {
    assert(cwd_);
    free(cwd_);
    memset(this, 0, sizeof(*this));
  }

  static UserProcess* current_;
 public:
  /*
   * A zombie process will have allocated_ being true (so the slot is not reused
   * by other process) and terminated_ being true.
   *
   * A wait system call from the parent process can release the slot.
   */
  bool allocated_;
  bool terminated_;
  int exit_status_; // this is set when terminated_ is set to true.
  int parent_pid_;  // it's -1 for processes created by kernel directly

  UserProcess* wait_for_child_;
  int* wait_for_pstatus_; // note, this is an address in user mode

  uint32_t pgdir_;
  InterruptFrame intr_frame_;

  // Store FileDesc* introduce one more indirection compared to storing FileDesc.
  // It's necessary so we can dupliate a opened file as the POSIX dup syscall
  // does.
  FileDescBase* filetab_[MAX_OPEN_FILE] = {nullptr};
  char* cwd_ = nullptr; // current working directory
};

extern uint32_t user_stack_start;
extern uint32_t user_process_va_start;

#ifdef __cplusplus
}
#endif
