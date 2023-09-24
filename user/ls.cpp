#include <stdio.h>
#include <syscall.h>
#include <dirent.h>

struct dirent entlist[1024];

int main(int argc, char **argv) {
  printf("Enter ls\n");

  // TODO default to current working directory instead
  const char* path = "/";
  if (argc >= 2) {
    path = argv[1];
  }

  int r = readdir(path, entlist, sizeof(entlist) / sizeof(entlist[0]));
  if (r < 0) {
    printf("Fail to readdir for '%s'\n", path);
    return -1;
  }
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
