#pragma once

#include <kernel/file_desc.h>

class PipeFileDesc;

#define PIPE_BUF_SIZE 4096

class Pipe {
 public:
  char* buf_; // a physical page
  PipeFileDesc* read_desc_;
  PipeFileDesc* write_desc_;
  int read_pos_;
  int write_pos_;
  int read(void* buf, int nbyte);
  int write(const void* buf, int nbyte);

  void init();
  void fini();
  PipeFileDesc* create_file_desc(bool write);

 private:
};

class PipeFileDesc : public FileDescBase {
 public:
  Pipe* pipeobj_;

  void init(Pipe* pipeobj, bool write);
  int read(void* buf, int nbyte);
  int write(const void* buf, int nbyte);
  void freeme();
 private:
};

int pipe(int fds[2]);
