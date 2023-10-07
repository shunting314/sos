#pragma once

#include <assert.h>

// relatively small for now
#define MAX_PATH_SIZE 256

// if both FD_FLAG_RD and FD_FLAG_RD are on, the file is opened for both read
// and write.
#define FD_FLAG_RD 1  // open for read
#define FD_FLAG_WR 2  // open for write
#define FD_FLAG_RW (FD_FLAG_RD | FD_FLAG_WR) // must be consistent with O_RDONLY/O_WRONLY defined in fcntl.h

// TODO: revise once we support virtual method
enum {
  FD_FILE,
  FD_CONSOLE,
};

class FileDesc;

class FileDescBase {
 public:
  int refcount_;
  int fdtype_;

  void decref() {
    if (--refcount_ == 0) {
      freeme();
    }
  }

  // TODO: because we don't support virtual method, these methods will be
  // manually dispatched to the corresponding child class.
  int read(void* buf, int nbyte);
  int write(const void* buf, int nbyte);
  void freeme();
};

class ConsoleFileDesc : public FileDescBase {
 public:
  ConsoleFileDesc() {
    refcount_ = 1;
    fdtype_ = FD_CONSOLE;
  }
  int read(void *buf, int nbyte);
  int write(const void* buf, int nbyte);
  // don't do anything
  void freeme() {}
};

// singleton
ConsoleFileDesc* acquire_console_file_desc();

class FileDesc : public FileDescBase {
 public:
  char path_[MAX_PATH_SIZE];
  int off_;
  int flags_;
  uint8_t* blkbuf_ = nullptr;

  int init(const char*path, int rwflags);

  void freeme();
  int read(void *buf, int nbyte);
  int write(const void* buf, int nbyte);

 private:
  FileDesc* next_;  // used for the freelist
  friend FileDesc* alloc_file_desc();
};

FileDesc* alloc_file_desc();
