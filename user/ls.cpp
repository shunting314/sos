#include <stdio.h>
#include <syscall.h>
#include <dirent.h>

struct dirent entlist[1024];

int main(void) {
  printf("Enter ls\n");

  // TODO get the path from cmdline argument
  int r = readdir("/", entlist, sizeof(entlist) / sizeof(entlist[0]));
  assert(r >= 0);
  if (r == 0) {
    printf("Empty dir\n");
    return 0;
  }
  for (int i = 0; i < r; ++i) {
    struct dirent* curEnt = &entlist[i];
    printf("  %s - type %s - %d bytes\n", curEnt->name, dirent_typestr(curEnt->ent_type), curEnt->file_size);
  }

  return 0;
}
