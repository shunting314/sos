#include <kernel/file_desc.h>
#include <kernel/phys_page.h>
#include <kernel/keyboard.h>
#include <kernel/simfs.h>
#include <string.h>
#include <stdlib.h>

#define MAX_FILE_DESC 4096

void setup_file_desc_freelist();

FileDesc all_file_desc[MAX_FILE_DESC];
static FileDesc* free_list;

struct _Initializer {
 public:
  explicit _Initializer() {
    setup_file_desc_freelist();
  }
} _initializer;

void setup_file_desc_freelist() {
  assert(free_list == nullptr);
  for (int i = MAX_FILE_DESC - 1; i >= 0; --i) {
    all_file_desc[i].freeme();
  }
}

FileDesc* alloc_file_desc() {
  assert(free_list && "Out of FileDesc");
  auto* ret = free_list;
  free_list = free_list->next_;
  return ret;
}

void decref_file_desc(FileDesc* fdptr) {
  if (--fdptr->refcount_ == 0) {
    fdptr->freeme();
  }
}

// will fail if path is too long
int FileDesc::init(const char* path, int rwflags) {
  fdtype_ = FD_FILE;
  refcount_ = 1;
  off_ = 0;
  flags_ = rwflags;
  int len = strlen(path);
  if (len > MAX_PATH_SIZE - 1) {
    return -1;
  }
  strcpy(path_, path);
  // blkbuf_ must have been reset to nullptr when the last user free the FileDesc
  assert(blkbuf_ == nullptr);
  return 0;
}

void FileDesc::freeme() {
  FileDesc* fdptr = this;
  // release the physical page if any is allocated
  if (blkbuf_) {
    free_phys_page((phys_addr_t) blkbuf_);
    blkbuf_ = nullptr;
  }
  fdptr->next_ = free_list;
  free_list = fdptr;
}

/*
 * Invariants: when blkbuf_ is not None, blkbuf_[off_ % BLOCK_SIZE] contains
 * the next byte of data.
 */
int FileDesc::read(void *buf, int nbyte) {
  // TODO: move majority of this code to class DirEnt
  assert(nbyte > 0);
  auto dent = SimFs::get().walkPath(path_); // TODO: should cache dent
  assert(dent);
  uint32_t file_size = dent.file_size;

  int tot_read = 0; 
  assert(off_ <= file_size);
  while (tot_read < nbyte && off_ != file_size) {
    if (!blkbuf_) {
      blkbuf_ = (uint8_t*) alloc_phys_page();
      uint32_t logical_blkid = off_ / BLOCK_SIZE;
      uint32_t phys_blkid = dent.logicalToPhysBlockId(logical_blkid);
      SimFs::get().readBlock(phys_blkid, blkbuf_);
    }
    assert(blkbuf_);

    // read content into buf_
    int tocpy = min(nbyte - tot_read, BLOCK_SIZE - off_ % BLOCK_SIZE);
    // the content in blkbuf_ does not necessarily all be valid if the file size
    // is not a multiple of BLOCK_SIZE. Use file_size as a cap
    tocpy = min(tocpy, file_size - off_);
    assert(tocpy > 0);
    memmove(buf + tot_read, &blkbuf_[off_ % BLOCK_SIZE], tocpy);
    tot_read += tocpy;
    off_ += tocpy;
    assert(off_ <= file_size);

    // check if blkbuf_ need to be released
    // We could alternatively read the next block
    if (off_ % BLOCK_SIZE == 0) {
      free_phys_page((phys_addr_t) blkbuf_);
      blkbuf_ = nullptr;
    }
  }
  return tot_read;
}

int FileDesc::write(const void *buf, int nbyte) {
  auto dent = SimFs::get().walkPath(path_);
  assert(dent);

  if (off_ + nbyte > dent.file_size) {
    dent.resize(off_ + nbyte);
    dent.flush(path_, strlen(path_));
  }
  dent.write(off_, nbyte, buf);
  off_ += nbyte;
  return nbyte;
}

// TODO: hack for the missing support of virtual method
#define FILE_DESC_DISPATCH(method, ...) \
  switch (fdtype_) { \
  case FD_FILE: \
    return ((FileDesc*) this)->method(__VA_ARGS__); \
  case FD_CONSOLE: \
    return ((ConsoleFileDesc*) this)->method(__VA_ARGS__); \
  default: \
    assert(false && "invalid fdtype_"); \
  }

int FileDescBase::read(void *buf, int nbyte) {
  FILE_DESC_DISPATCH(read, buf, nbyte);
}

int FileDescBase::write(const void* buf, int nbyte) {
  FILE_DESC_DISPATCH(write, buf, nbyte);
}

void FileDescBase::freeme() {
  FILE_DESC_DISPATCH(freeme);
}

ConsoleFileDesc g_console_file_desc;
ConsoleFileDesc* acquire_console_file_desc() {
  ++g_console_file_desc.refcount_;
  return &g_console_file_desc;
}

/*
 * TODO hacky: single character with negative value represents keep reading.
 * Need be consistent with read function in ulib/syscall.cpp .
 */
int ConsoleFileDesc::read(void *buf, int nbyte) {
  assert(nbyte > 0);
  char* s = (char*) buf;
  int cnt = 0;
  while (cnt < nbyte) {
    char ch = keyboardGetChar(false);
    if (ch == 0) {
      break;
    }
    s[cnt++] = ch;
    if (ch == KEYBOARD_CTRL_D) {
      break;
    }
  }

  if (cnt == 0) {
    // need client keep reading
    s[0] = -1;
    return 1;
  }

  if (cnt >= 2 && s[cnt - 1] == KEYBOARD_CTRL_D) {
    // put back ctrl-d
    keyboardPutback(KEYBOARD_CTRL_D);
    return cnt - 1;
  }

  if (cnt == 1 && s[cnt - 1] == KEYBOARD_CTRL_D) {
    return 0; // EOF
  }

  return cnt;
}

int ConsoleFileDesc::write(const void* buf, int nbyte) {
  char *s = (char*) buf;
  for (int i = 0; i < nbyte; ++i) {
    putchar(s[i]);
  }
  return nbyte;
}
