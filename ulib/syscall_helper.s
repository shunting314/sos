# TODO: naming this file syscall.s will cause trouble since we already have
# a syscall.cpp under the same folder. They both generate syscall.o...
# One neat solution is to call the former object file syscall.s.o, and the latter
# syscall.cpp.o. But we need change Makefile to apply this convention everywhere
.global syscall
syscall:
  # TODO replace this file with inline assembly
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
