#include <string.h>
#include <fcntl.h>
#include <syscall.h>

int main(void) {
  int fd = open("/newmessage", O_WRONLY);
  assert(fd >= 0);
  printf("fd is %d\n", fd);
  char buf[128] = "This is a new message\n";
  int r = write(fd, buf, strlen(buf));
  assert(r == strlen(buf));
  close(fd);


  int fd2 = open("/largefile", O_WRONLY);
  assert(fd2 >= 0);
  printf("fd2 is %d\n", fd2);
  char buf2[4096 * 2]; // 2 blocks
  for (int i = 0; i < sizeof(buf2); ++i) {
    buf2[i] = 'a' + (i % 26);
  }
  r = write(fd2, buf2, sizeof(buf2));
  assert(r == sizeof(buf2));
  close(fd2);

  // verify the content
  int fd_read = open("/largefile", O_RDONLY);
  printf("fd_read is %d\n", fd_read);
  char buf3[4096 * 2];
  r = read(fd_read, buf3, sizeof(buf3));
  assert(r == sizeof(buf3));
  for (int i = 0; i < sizeof(buf3); ++i) {
    assert(buf3[i] == (i % 26) + 'a');
  }
  assert(read(fd_read, buf3, sizeof(buf3)) == 0); // no more left
  close(fd_read);
  printf("bye!\n");
  return 0;
}
