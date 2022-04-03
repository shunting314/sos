.global asm_lidt
asm_lidt:
  lidt idtrdesc
  ret

.global asm_sti
asm_sti:
  sti
  ret

.global asm_return_from_interrupt
asm_return_from_interrupt:
  mov 4(%esp), %esp
  popa
  addl $4, %esp # skip the error code
  iret

.global asm_outb
asm_outb:
  movw 4(%esp), %dx
  movb 8(%esp), %al
  outb %al, %dx
  ret

.global asm_inb
asm_inb:
  movw 4(%esp), %dx
  xor %eax, %eax
  inb %dx, %al
  ret

.global asm_outw
asm_outw:
  movw 4(%esp), %dx
  movw 8(%esp), %ax
  outw %ax, %dx
  ret

.global asm_inw
asm_inw:
  movw 4(%esp), %dx
  xor %eax, %eax
  inw %dx, %ax
  ret

.global asm_set_cr3
asm_set_cr3:
  movl 4(%esp), %eax
  movl %eax, %cr3
  ret

.global asm_cr0_enable_flags
asm_cr0_enable_flags:
  movl %cr0, %eax
  orl 4(%esp), %eax
  movl %eax, %cr0
  ret

.global asm_enter_user_mode
asm_enter_user_mode:
  push %ebp
  mov %esp, %ebp

  push $35 # user ss
  mov 8(%ebp), %eax
  push %eax # esp
  # enable IF
  push $0x200
  push $27 # user cs
  mov 12(%ebp), %eax
  push %eax # eip

  # set other segment registers
  push $35
  pop %ds
  push %ds
  pop %es
  push %ds
  pop %fs
  push %ds
  pop %gs
  iret

.global asm_load_tr
asm_load_tr:
  mov $40, %ax
  ltr %ax
  ret
