#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include <dirent.h>

void usage() {
  printf("Usage: rm [-r] path\n");
}

int rmdirRecursively(const char* path) {
  // TODO add a syscall to check if path is a regular file or directory.
  // Return -1 directly if it's a regular file.

  // TODO take care of stack overflow for the recursive calls.

  struct dirent entlist[32]; // TODO does not work if a directory contains more than 32 entries.
  int cnt = readdir(path, entlist, sizeof(entlist) / sizeof(*entlist));
  if (cnt < 0) {
    return cnt;
  }

  char fullpath[256]; // TODO use malloc to handle longer path

  int pathlen = strlen(path);
  assert(pathlen < sizeof(fullpath) / sizeof(*fullpath));
  strcpy(fullpath, path);
  fullpath[pathlen++] = '/';
  int r;
  for (int i = 0; i < cnt; ++i) {
    struct dirent* ent = &entlist[i];
    int entlen = strlen(ent->name);
    assert(pathlen + entlen < sizeof(fullpath) / sizeof(*fullpath));
    strcpy(fullpath + pathlen, ent->name);

    if (ent->ent_type == ET_DIR) {
      r = rmdirRecursively(fullpath);
      if (r < 0) {
        return r;
      }
    } else if (ent->ent_type == ET_FILE) {
      r = unlink(fullpath);
      if (r < 0) {
        return r;
      }
    } else {
      assert(false && "invalid ent type");
    }
  }

  // remove the empty directory here
  return rmdir(path);
}

/*
 * Only support 2 cases right now:
 * 1. rm file-path
 * 2. rm -r dir-path
 */
int main(int argc, char **argv) {
  bool isdir = false;
  const char *path = nullptr;
  if (argc == 2) {
    path = argv[1];
  } else if (argc == 3) {
    if (strcmp(argv[1], "-r") != 0) {
      usage();
      return -1;
    }
    path = argv[2];
    isdir = true;
  } else {
    usage();
    return -1;
  }

  assert(path);

  int r;
  if (isdir) {
    r = rmdirRecursively(path);
    if (r < 0) {
      printf("Fail to remove directory %s\n", path);
      return r;
    }
  } else {
    r = unlink(path);
    if (r < 0) {
      printf("Fail to remove file %s\n", path);
      return r;
    }
  }
  printf("Succeed removing path %s\n", path);
  return 0;
}
