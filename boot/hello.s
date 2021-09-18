.code16
.org 0x7c00
  # clear the screen by setting the video mode
  movb $0, %ah
  movb $3, %al
  int $0x10

  # count the length of the message string
  movw $msg, %si
  xorw %cx, %cx
loop_begin:
  movb (%si), %al
  testb %al, %al
  jz loop_end
  incw %si
  incw %cx
  jmp loop_begin
loop_end:
  # print the message
  movb $1, %al
  movb $0, %bh
  movb $0x0e, %bl # specifies the foreground and background color
  movb $0, %dh # row, start from 0
  movb $0, %dl # column, start from 0
  pushw %cs
  popw %es
  movw $msg, %bp
  movb $0x13, %ah
  int $0x10

msg:
  .ascii "Welcome to my hello-world bootloader!\r\n"
  .asciz "The journey of writing my own operating system SOS has just started!"
