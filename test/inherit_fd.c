/*
 * The open file descriptor in parent process is inherited by the child process.
 * When sharing the file descriptor, the position reading/writing the file
 * is also shared. Thus we see the parent and child process read interleaved
 * content.
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

int main(int argc, char** argv) {
  const char* path = __FILE__;
  int fd = open(path, O_RDONLY);
  assert(fd >= 0);
  char buf[5] = {0};
  int fork_ret = fork();
  assert(fork_ret >= 0);
  if (fork_ret == 0) {
    // child
    for (int i = 0; i < 3; ++i) {
      read(fd, buf, 4);
      printf("child: %s\n", buf);
      sleep(1);
    }
  } else {
    // parent
    for (int i = 0; i < 3; ++i) {
      read(fd, buf, 4);
      printf("parent: %s\n", buf);
      sleep(1);
    }
  }
  return 0;
}
