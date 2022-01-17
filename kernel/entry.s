.globl entry
entry:
  cli
  movl $stack_top, %esp
  call kernel_main
1:
  jmp 1b

# setup a larger kernel stack
.data
.align 4096
stack_bottom:
  .space 65536 # 64K for now
stack_top:
