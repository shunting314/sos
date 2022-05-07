#include <syscall_no.h>

// place holder argument to pad syscall arg list size to 5
#define PHARG 0
extern "C" int syscall(int no, int arg1, int arg2, int arg3, int arg4, int arg5);

#if 0
int syscall(int no, int arg1, int arg2, int arg3, int arg4, int arg5) {
  // TODO implement this with inline assembly
}
#endif

int write(int fd, char* buf, int sz) {
  return syscall(SC_WRITE, fd, (int) buf, sz, PHARG, PHARG);
}
