/*
 * There is no significant reason why put this in a C++ file rather than an asm file.
 * I mainly want to show that basic inline assembly is quite powerful. We can embed
 * a large chunk of non-trivial assembly code into a C++ file.
 */
asm(R"(
.global entry
entry:
  cli
  movl $kernel_stack_top, %esp

  lgdt gdt_desc
  jmp $CODE_SEG, $reset_cs
reset_cs:
  mov $DATA_SEG, %ax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs
  mov %ax, %ss
  call kernel_main
1:
  jmp 1b

.equ CODE_SEG, 8
.equ DATA_SEG, 16

  .align 8
gdt:
  .long 0, 0 # null segment
  .long 0x0000FFFF, 0x00CF9A00 # kernel code segment
  .long 0x0000FFFF, 0x00CF9200 # kernel data segment
  .long 0x0000FFFF, 0x00CFFA00 # user code segment
  .long 0x0000FFFF, 0x00CFF200 # user data segment
.global tss_segment_desc
tss_segment_desc:
  .long 0, 0 # placeholder for TSS
gdt_end:

gdt_desc:
  .word gdt_end - gdt - 1
  .long gdt

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
kernel_stack_bottom:
  .space 65536 # 64K for now
.global kernel_stack_top
kernel_stack_top:

# define kernel_page_dir in assembly since it's easier to control the alignment
.global kernel_page_dir
kernel_page_dir:
  .space 4096
)");
