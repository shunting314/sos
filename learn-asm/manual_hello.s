  .globl _main
_main:
  pushq %rbp
  movq %rsp, %rbp
  leaq mymsg(%rip), %rdi # don't really know why need use %rip here yet
  call _printf
  popq %rbp
  movq $123, %rax
  ret

mymsg:
  .asciz "Hello, world!\n"
