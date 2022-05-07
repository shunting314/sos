# TODO make this entry file general and put in ulib
.global entry
entry:
  call main

  # TODO we should call some syscall to exit the process
1:
  jmp 1b
