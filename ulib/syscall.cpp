#include <syscall_no.h>
#include <syscall.h>

// place holder argument to pad syscall arg list size to 5
#define PHARG 0

extern "C" int syscall(int no, int arg1, int arg2, int arg3, int arg4, int arg5);
asm(R"(
  .global syscall
syscall:
  push %ebp
  mov %esp, %ebp

  mov 8(%ebp), %eax
  mov 12(%ebp), %ebx
  mov 16(%ebp), %ecx
  mov 20(%ebp), %edx
  mov 24(%ebp), %esi
  mov 28(%ebp), %edi
  int $0x30

  mov %ebp, %esp
  pop %ebp
  ret
)");

int write(int fd, const char* buf, int sz) {
  return syscall(SC_WRITE, fd, (int) buf, sz, PHARG, PHARG);
}

int exit(int status) {
  return syscall(SC_EXIT, status, PHARG, PHARG, PHARG, PHARG);
}

int dumbfork() {
  return syscall(SC_DUMBFORK, PHARG, PHARG, PHARG, PHARG, PHARG);
}

int fork() {
  return syscall(SC_FORK, PHARG, PHARG, PHARG, PHARG, PHARG);
}

int spawn(const char* path) {
  return syscall(SC_SPAWN, (int) path, PHARG, PHARG, PHARG, PHARG);
}

int getpid() {
  return syscall(SC_GETPID, PHARG, PHARG, PHARG, PHARG, PHARG);
}

int open(const char*path, int oflags) {
	return syscall(SC_OPEN, (int) path, oflags, PHARG, PHARG, PHARG);
}

int read(int fd, void *buf, int nbyte) {
  return syscall(SC_READ, fd, (int) buf, nbyte, PHARG, PHARG);
}

int close(int fd) {
  return syscall(SC_CLOSE, fd, PHARG, PHARG, PHARG, PHARG);
}

/*
 * Note we return the child process's exit status in *pstatus directly.
 * This may not be compatible with POSIX.
 */
int waitpid(int pid, int *pstatus, int /* options */) {
  return syscall(SC_WAITPID, pid, (int) pstatus, PHARG, PHARG, PHARG);
}
