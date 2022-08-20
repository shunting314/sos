#include <stdio.h>
#include <syscall.h>
#include <fcntl.h>
#include <assert.h>

// TODO support argc/argv for kernel launch cmd
int main(void) {
  printf("Enter the main function of shell\n");

  int nonexist_fd = open("/non-exist-file", O_RDONLY);
  assert(nonexist_fd < 0);

  printf("nonexist_fd %d\n", nonexist_fd);

  int fd = open("/message", O_RDONLY);
  assert(fd >= 0);
  printf("fd is %d\n", fd);
  printf("bye\n");
  return 0;
}
