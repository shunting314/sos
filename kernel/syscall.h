#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#include <syscall_no.h>

#ifdef __cplusplus
extern "C" {
#endif

int syscall(int sc_no, int arg1, int arg2, int arg3, int arg4, int arg5);

#ifdef __cplusplus
}
#endif

#endif
