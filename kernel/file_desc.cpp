#include <kernel/file_desc.h>
#include <string.h>

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
  refcount_ = 1;
  off_ = 0;
  flags_ = rwflags;
  int len = strlen(path);
  if (len > MAX_PATH_SIZE - 1) {
    return -1;
  }
  strcpy(path_, path);
  return 0;
}

void FileDesc::freeme() {
  FileDesc* fdptr = this;
  fdptr->next_ = free_list;
  free_list = fdptr;
}
