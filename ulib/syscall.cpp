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

  # %eax, %ecx, %edx does not need to be saved since
  # the caller is responsible to save them if needed
  push %ebx
  push %esi
  push %edi

  mov 8(%ebp), %eax
  mov 12(%ebp), %ebx
  mov 16(%ebp), %ecx
  mov 20(%ebp), %edx
  mov 24(%ebp), %esi
  mov 28(%ebp), %edi
  int $0x30

  pop %edi
  pop %esi
  pop %ebx

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

int spawn(const char* path, const char** argv, int fdin, int fdout) {
  return syscall(SC_SPAWN, (int) path, (int) argv, fdin, fdout, PHARG);
}

int pipe(int fds[2]) {
  return syscall(SC_PIPE, (int) fds, PHARG, PHARG, PHARG, PHARG);
}

int getpid() {
  return syscall(SC_GETPID, PHARG, PHARG, PHARG, PHARG, PHARG);
}

int open(const char*path, int oflags) {
	return syscall(SC_OPEN, (int) path, oflags, PHARG, PHARG, PHARG);
}

int read(int fd, void *buf, int nbyte) {
  int r;
  while (true) {
    r = syscall(SC_READ, fd, (int) buf, nbyte, PHARG, PHARG);
    // a hack representing keep reading for keyboard input
    if (r == 1 && ((char*) buf)[0] < 0) {
      continue; 
    }
    break;
  }
  return r;
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

int readdir(const char*path, struct dirent* entlist, int capa) {
  return syscall(SC_READDIR, (int) path, (int) entlist, capa, PHARG, PHARG);
}

int mkdir(const char* path) {
  return syscall(SC_MKDIR, (int) path, PHARG, PHARG, PHARG, PHARG);
}

char* getcwd(char* path, int len) {
  int r = syscall(SC_GETCWD, (int) path, len, PHARG, PHARG, PHARG);
  if (r >= 0) {
    return path;
  } else {
    return nullptr;
  }
}

int chdir(const char* path) {
  return syscall(SC_CHDIR, (int) path, PHARG, PHARG, PHARG, PHARG);
}
