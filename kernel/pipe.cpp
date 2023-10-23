#include <kernel/pipe.h>
#include <kernel/phys_page.h>
#include <kernel/user_process.h>
#include <assert.h>
#include <stdlib.h>

void Pipe::init() {
  buf_ = (char*) alloc_phys_page();
  read_desc_ = create_file_desc(false);
  write_desc_ = create_file_desc(true);
  read_pos_ = write_pos_ = 0;
}

void Pipe::fini() {
  free_phys_page((phys_addr_t) buf_);
  buf_ = nullptr;
  assert(read_desc_ == nullptr);
  assert(write_desc_ == nullptr);
  printf("Destructing a pipe object\n");
}

PipeFileDesc* Pipe::create_file_desc(bool write) {
  PipeFileDesc* desc = (PipeFileDesc*) malloc(sizeof(PipeFileDesc));
  desc->init(this, write);
  return desc;
}

int Pipe::read(void* buf, int nbyte) {
  assert(nbyte > 0);
  char* s = (char*) buf;
  if (read_pos_ == write_pos_) {
    if (write_desc_) {
      // TODO: hack. waiting for data
      s[0] = -1;
      return 1;
    } else {
      // no data and no writer, return EOF
      return 0;
    }
  }
  int cnt = 0;
  while (cnt < nbyte && read_pos_ < write_pos_) {
    s[cnt++] = buf_[read_pos_++ % PIPE_BUF_SIZE];
  }
  return cnt;
}

int Pipe::write(const void* buf, int nbyte) {
  char *s = (char*) buf;
  int cnt = 0;
  while (cnt < nbyte && write_pos_ - read_pos_ < PIPE_BUF_SIZE) {
    buf_[write_pos_++ % PIPE_BUF_SIZE] = s[cnt++];
  }
  return cnt;
}

void PipeFileDesc::init(Pipe* pipeobj, bool write) {
  refcount_ = 1;
  fdtype_ = FD_PIPE;
  flags_ = write ? FD_FLAG_WR : FD_FLAG_RD;
  pipeobj_ = pipeobj;
}

// return 0 represent EOF
int PipeFileDesc::read(void* buf, int nbyte) {
  return pipeobj_->read(buf, nbyte);
}

int PipeFileDesc::write(const void* buf, int nbyte) {
  return pipeobj_->write(buf, nbyte);
}

void PipeFileDesc::freeme() {
  if (flags_ == FD_FLAG_RD) {
    printf("Free pipe read end\n");
    assert(pipeobj_->read_desc_ == this);
    pipeobj_->read_desc_ = nullptr;
  } else if (flags_ == FD_FLAG_WR) {
    printf("Free pipe write end\n");
    assert(pipeobj_->write_desc_ == this);
    pipeobj_->write_desc_ = nullptr;
  } else {
    assert(false && "can not reach here");
  }

  if (!pipeobj_->read_desc_ && !pipeobj_->write_desc_) {
    pipeobj_->fini();
  }
  free(this);
}

int pipe(int fds[2]) {
  Pipe* pipeobj = (Pipe*) malloc(sizeof(*pipeobj));
  pipeobj->init();
  auto cur = UserProcess::current();

  fds[0] = cur->allocFd(pipeobj->read_desc_);
  fds[1] = cur->allocFd(pipeobj->write_desc_);
  return 0;
}
