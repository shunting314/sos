.global entry
entry:
  cli
  movl $stack_top, %esp
  call kernel_main
1:
  jmp 1b

# setup a quite large kernel stack
# There was a tricky bug. boot/bootloader.s has limitation that we load at most
# the first 64K from the kernel. Setting up a 64K size stack in data segment
# will consume 64K size in the kernel ELF file. This cause global/static variables
# comming after the stack space in the ELF file not being loaded thus by default be all 0..
# Before we fix the limitation in boot/bootloader.s, put the large kernel stack
# in .bss section to avoid consuming kernel elf file size. Even after we improve
# the bootloader to be able to loading kernel of any size, it's still good to
# not consume ELF file space by putting the stack in .bss section.
# .data
.bss
.align 4096
stack_bottom:
  .space 65536 # 64K for now
stack_top:
