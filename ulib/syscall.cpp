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

int write(int fd, char* buf, int sz) {
  return syscall(SC_WRITE, fd, (int) buf, sz, PHARG, PHARG);
}

// TODO should we accept 1 argument as the exit status?
int exit() {
  return syscall(SC_EXIT, PHARG, PHARG, PHARG, PHARG, PHARG);
}
