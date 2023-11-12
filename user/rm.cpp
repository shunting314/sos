#include <stdio.h>
#include <string.h>
#include <syscall.h>

void usage() {
  printf("Usage: rm [-r] path\n");
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
  } else {
    usage();
    return -1;
  }

  if (isdir) {
    printf("Only support removing file at the moment\n");
    return -1;
  }
  int r;
  assert(path);
  r = unlink(path);
  if (r < 0) {
    printf("Fail to remove %s\n", path);
    return r;
  }
  printf("Succeed removing path %s\n", path);
  return 0;
}
