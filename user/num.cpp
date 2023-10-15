/*
 * Read text from stdin, add line number to each line and write the result to
 * stdout. Used to test io redirection.
 *
 * TODO make readline an API.
 * TODO dedup code with cat
 * TODO support width for printf
 */
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <syscall.h>

int main(int argc, char** argv) {
  int fd = 0;
  if (argc >= 2) {
    fd = open(argv[1], O_RDONLY);
  }
  if (fd < 0) {
    printf("Fail to open file %s\n", argv[1]);
    return -1;
  }
  // we could reuse the memory in buf, but having a separate buffer for
  // printing is easier to implement.
  char line[128];
  int lineoff = 0;
  char buf[128];
  int lineno = 1;
  int r;
  while ((r = read(fd, buf, sizeof(buf) - 1)) != 0) {
    assert(r > 0);
    buf[r] = '\0';

    int i = 0;
    while (true) {
      while (i < r && lineoff < sizeof(line) - 1) {
        line[lineoff++] = buf[i++];
        if (buf[i - 1] == '\n') {
          break;
        }
      }
      if (lineoff == sizeof(line) - 1 || (lineoff > 0 && line[lineoff - 1] == '\n')) {
        line[lineoff] = '\0';
        printf("%d: ", lineno++);
        printf("%s", line);
        lineoff = 0;
        continue;
      }
      assert(i == r);
      break;
    }
  }
  if (lineoff > 0) {
    line[lineoff] = '\0';
    printf("%d: ", lineno++);
    printf("%s\n", line);
  }

  if (fd != 0) {
    close(fd);
  }
  return 0;

}
