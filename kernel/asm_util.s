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
  iret

.global asm_outb
asm_outb:
  movw 4(%esp), %dx
  movb 8(%esp), %al
  outb %al, %dx
  ret
