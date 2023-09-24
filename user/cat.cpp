// TODO: add support for cat the stdin
#include <stdio.h>
#include <fcntl.h>
#include <syscall.h>
#include <assert.h>

int main(int argc, char**argv) {
  if (argc != 2) {
    printf("Missing a path argument\n");
    return -1;
  }

  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    printf("Fail to open file %s\n", argv[1]);
    return -1;
  }
  char buf[128];
  int r;
  while ((r = read(fd, buf, sizeof(buf) - 1)) != 0) {
    assert(r > 0);
    buf[r] = '\0';
    printf("%s", buf);
  }
  printf("\n");

  close(fd);
  return 0;
}
