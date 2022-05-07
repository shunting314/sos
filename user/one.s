.global entry
entry:
  nop
1:
  mov $1, %eax # syscall number 1 for write
  mov $1, %ebx # file descriptor
  mov $msg, %ecx
  mov $(msg_end - msg), %edx # cnt
  mov $0, %esi
  mov $0, %edi
  int $0x30
  jmp 1b

msg:
  .ascii "From Tao there comes one. From one there comes two.\n"
msg_end:
