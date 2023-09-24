#include <assert.h>
#include <stdio.h>
#include <syscall.h>

int main(int argc, char** argv) {
  if (argc != 2) {
    printf("Usage: mkdir [path]\n");
    return -1;
  }
  char* path = argv[1];
  int status = mkdir(path);
  if (status < 0) {
    printf("Failed to create directory %s\n", path);
    return -1;
  }
  return 0;
}
