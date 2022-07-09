.globl entry
entry:

.code16
  cli
  movw $0xA000, %sp # setup the stack

  # enter protected mode
  lgdt gdt_desc
  movl %cr0, %eax
  orw $1, %ax
  movl %eax, %cr0

  jmp $CODE_SEG, $protected_mode
.code32
protected_mode:
  mov $DATA_SEG, %ax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs
  mov %ax, %ss

  xorl %ebp, %ebp

  call load_kernel_and_enter
1:
  jmp 1b

.equ CODE_SEG, 8
.equ DATA_SEG, 16

  .align 8
gdt:
  .long 0, 0 # null segment
  .long 0x0000FFFF, 0x00CF9A00 # code segment
  .long 0x0000FFFF, 0x00CF9200 # data segment
gdt_end:

gdt_desc:
  .word gdt_end - gdt - 1
  .long gdt
