#include <kernel/syscall.h>
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

void *sc_handlers[NUM_SYS_CALL] = {
  [SC_WRITE] = sys_write,
};

typedef int (*sc_handler_type)(int arg1, int arg2, int arg3, int arg4, int arg5);

int syscall(int sc_no, int arg1, int arg2, int arg3, int arg4, int arg5) {
  // printf("Handle syscall %d, args[0x%x, 0x%x, 0x%x, 0x%x, 0x%x]\n", sc_no, arg1, arg2, arg3, arg4, arg5);
  assert(sc_no < NUM_SYS_CALL && sc_handlers[sc_no]);
  return ((sc_handler_type)sc_handlers[sc_no])(arg1, arg2, arg3, arg4, arg5);
}
