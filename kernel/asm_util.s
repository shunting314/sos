.global asm_lidt
asm_lidt:
  lidt idtrdesc
  ret
