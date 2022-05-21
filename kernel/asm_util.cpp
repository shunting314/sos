/*
 * NOTE: I define asm_set_cr3 as a function rather than extended inline assembly
 * since 'cr3' can not be specified on the clobber list.
 */
asm(R"(
.global asm_set_cr3
asm_set_cr3:
  movl 4(%esp), %eax
  movl %eax, %cr3
  ret
)");

asm(R"(
.global asm_return_from_interrupt
asm_return_from_interrupt:
  mov 4(%esp), %esp
  popa
  addl $4, %esp # skip the error code
  iret
)");

asm(R"(
.global asm_cr0_enable_flags
asm_cr0_enable_flags:
  movl %cr0, %eax
  orl 4(%esp), %eax
  movl %eax, %cr0
  ret
)");

asm(R"(
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
)");

asm(R"(
.global asm_load_tr
asm_load_tr:
  mov $40, %ax
  ltr %ax
  ret
)");

asm(R"(
.global asm_lidt
asm_lidt:
  lidt idtrdesc
  ret
)");
