# The bootloader does the following things
# 1. leverage BIOS to read kernel from disk. This has to be done in real mode
# 3. enter protected mode
# 3. load the ELF format kernel. It's better to be done in C to handle ELF format
# 4. jump into the kernel
.globl entry
entry:
.code16
  cli
  movw $0xA000, %sp # setup the stack

  # clear the screen by setting the video mode
  movb $0, %ah
  movb $3, %al
  int $0x10

  # load disk regions from sector2 to address start from 0x10000
  pushw $0x1000 # can NOT move immediate number to es directly. Thus use the stack.
  popw %es
  movw $0, %bx
  movb $128, %al # TODO don't hardcode here. 128 sectors are 64K in size.
  movb $0, %ch
  movb $2, %cl
  movb $0, %dh
  movb $0x80, %dl
  movb $2, %ah
  int $0x13
  # check for error: need check CF and AH
  jc disk_error
  testb %ah, %ah
  jnz disk_error

  call print_sth

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
  # keep eip the same

  call load_kernel_and_enter
1:
  jmp 1b

disk_error:
  jmp disk_error

# function
print_sth:
  movb $0xe, %ah
  movb $'>', %al
  int $0x10
  movb $'8', %al
  int $0x10
  movb $' ', %al
  int $0x10
  call print_nl
  ret

print_nl:
  movb $0xe, %ah
  movb $0x0d, %al
  # movb $'\r', %al # $'\r' does not work somehow
  int $0x10
  movb $'\n', %al
  int $0x10
  ret

.equ CODE_SEG, 8
.equ DATA_SEG, 16

gdt:
  .long 0, 0 # null segment
  .long 0x0000FFFF, 0x00CF9A00 # code segment
  .long 0x0000FFFF, 0x00CF9200 # data segment
gdt_end:

gdt_desc:
  .word gdt_end - gdt - 1
  .long gdt
