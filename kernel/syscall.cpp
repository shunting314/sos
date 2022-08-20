#include <kernel/syscall.h>
#include <kernel/user_process.h>
#include <kernel/fork.h>
#include <kernel/fileapi.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

int sys_write(int fd, void *buf, int cnt) {
  char *s = (char*) buf;
  for (int i = 0; i < cnt; ++i) {
    putchar(s[i]);
  }
  return cnt;
}

// terminate the current process
int sys_exit() {
  UserProcess::terminate_current_process();
  assert(false && "never reach here");
  return 0;
}

int sys_dumbfork() {
  return dumbfork();
}

int sys_fork() {
  return cowfork();
}

int sys_getpid() {
  return UserProcess::current()->get_pid();
}

int sys_open(const char*path, int oflags) {
	return file_open(path, oflags);	
}

void *sc_handlers[NUM_SYS_CALL] = {
  nullptr, // number 0
  // "[SC_WRITE] = fptr; " seems work in C but is not supported by C++.
  // We need be super carefully with the order.
  /* [SC_WRITE] = */ (void*) sys_write,
  /* [SC_EXIT] = */ (void*) sys_exit,
  /* SC_DUMBFORK */ (void*) sys_dumbfork,
  /* SC_FORK */ (void*) sys_fork,
  /* SC_GETPID */ (void*) sys_getpid,
	/* SC_OPEN */ (void*) sys_open,
};

typedef int (*sc_handler_type)(int arg1, int arg2, int arg3, int arg4, int arg5);

int syscall(int sc_no, int arg1, int arg2, int arg3, int arg4, int arg5) {
  // printf("Handle syscall %d, args[0x%x, 0x%x, 0x%x, 0x%x, 0x%x]\n", sc_no, arg1, arg2, arg3, arg4, arg5);
  assert(sc_no < NUM_SYS_CALL && sc_handlers[sc_no]);
  return ((sc_handler_type)sc_handlers[sc_no])(arg1, arg2, arg3, arg4, arg5);
}
