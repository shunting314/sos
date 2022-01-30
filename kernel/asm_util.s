.global asm_lidt
asm_lidt:
  lidt idtrdesc
  ret

.global asm_return_from_interrupt
asm_return_from_interrupt:
  mov 4(%esp), %esp
  iret
