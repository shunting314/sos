.code16
.org 0x7c00
  # clear the screen by setting the video mode
  movb $0, %ah
  movb $3, %al
  int $0x10

  # setup stack for function calls
  movw $0xa000, %sp # 0x9xxx for the stack
  xorw %bp, %bp

  # print some characters
  call print_sth

  # load some sectors from the disk
  # setup es:bx which points to data buffer
  pushw $3 # number of sectors to read
  pushw $0x8000 # buffer address
  call load_disk
  addw $4, %sp

  call print_nl

  pushw $0x8000
  call print_str
  addw $2, %sp
  pushw $0x8200
  call print_str
  addw $2, %sp
  pushw $0x8400
  call print_str
  addw $2, %sp

1:
  hlt
  jmp 1b

# function
print_sth:
  movb $0xe, %ah
  movb $'>', %al
  int $0x10
  movb $'8', %al
  int $0x10
  call print_nl
  ret

# function
print_nl:
  movb $0xe, %ah
  movb $0x0d, %al
  # movb $'\r', %al # $'\r' does not work somehow
  int $0x10
  movb $'\n', %al
  int $0x10
  ret

# function: arguments: addr, num_of_sectors
load_disk:
  pushw %bp
  movw %sp, %bp

  movw 4(%bp), %bx
  movb 6(%bp), %al

  movb $0, %ch
  movb $2, %cl
  movb $0, %dh
  movb $0x80, %dl
  movb $2, %ah
  int $0x13

  # check for error: need check CF and AH
  jc print_error
  testb %ah, %ah
  jnz print_error

  movb $0xe, %ah
  movb $'S', %al
  int $0x10
  
  jmp load_disk_out
print_error:
  movb $0xe, %ah
  movb $'E', %al
  int $0x10
1:
  jmp 1b # loop here is error happens

load_disk_out:
  movw %bp, %sp
  popw %bp
  ret

# function: argument: str
print_str:
  pushw %bp
  movw %sp, %bp

  movw 4(%bp), %bx
  movb $0xe, %ah
print_loop_begin:
  movb (%bx), %al 
  testb %al, %al
  jz print_loop_end
  int $0x10
  incw %bx
  jmp print_loop_begin

print_loop_end:
  movw %bp, %sp
  popw %bp
  ret
