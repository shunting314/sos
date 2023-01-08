.globl entry
entry:
.code16
  nop
  cli

  # required on Toshiba
  xor %ax, %ax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs
  mov %ax, %ss

  movw $0xA000, %sp # setup the stack

  # reset video mode
  mov $0x0003, %ax
  int $0x10

  mov 0x7c00, %bl
  call print_byte

  # read sectors  using LBA extension
  # 0x500 sections result in 0xA0000 bytes which is the max kernel size supported by SOS
  # without counting extensions.
  mov $0x500, %cx

  # read 1 sector for each iteration
read_disk:
  mov $packet, %si
  mov $0x42, %ah
  mov $0x80, %dl
  int $0x13

  incw LBA
  addw $32, SEG  # advance 32 * 16 = 512 bytes
  dec %cx
  jnz read_disk

  # enter protected mode
  lgdt gdt_desc
  movl %cr0, %eax
  orw $1, %ax
  movl %eax, %cr0

  jmp $CODE_SEG, $protected_mode

  # put real mode data structure and code here before we do
  # .code32 assembly below when entering protected mode.
.align 4
packet:
  .byte 16
  .byte 0
  .word 1 # number of sector
  .word 0 # offset
SEG:
  .word 0x1000 # segment
LBA:
  .long 0x1 # lower 32 bit of LBA
  .long 0 # upper 16 bit of LBA

  # Utilities to print a byte on the screen. Super helpful for debugging
  # on the real hardward. Can remove this if we reach MBR size limit.
xdig_table:
  .ascii "0123456789abcdef"
print_byte:
  # input byte is from bl
  rol $4, %bl
  mov %bl, %al
  and $0xf, %al
  mov $xdig_table, %bp
  mov $0, %ah
  add %ax, %bp
  mov (%bp), %al
  mov $0xe, %ah
  int $0x10
  
  rol $4, %bl
  mov %bl, %al
  and $0xf, %al
  mov $0, %ah
  mov $xdig_table, %bp
  add %ax, %bp
  mov (%bp), %al
  mov $0xe, %ah
  int $0x10
  ret

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
