#include <stdio.h>
#include <syscall.h>
#include <fcntl.h>
#include <assert.h>

// TODO support argc/argv for kernel launch cmd
extern "C" int mymain(void) {
  int nonexist_fd = open("/non-exist-file", O_RDONLY);
  assert(nonexist_fd < 0);

  printf("nonexist_fd %d\n", nonexist_fd);

  int fd = open("/message", O_RDONLY);
  assert(fd >= 0);
  printf("fd is %d\n", fd);

	char buf[128];
	int r;
	// read the file content
	printf("File content:\n");
	while ((r = read(fd, buf, sizeof(buf) - 1)) != 0) {
		assert(r > 0);
		buf[r] = '\0';
		printf("%s", buf);
	}
	printf("\n");

  close(fd);

  printf("bye\n");
  return 0;
}
