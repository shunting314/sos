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
