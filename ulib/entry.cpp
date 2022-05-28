#include <app_init_state.h>
#include <stdio.h>

// TODO support global destructor
asm(R"(
.global entry
entry:
  push %esp
  call premain
  add $4, %esp

  call main
  # TODO we should call some syscall to exit the process
1:
  jmp 1b
)");

/*
 * Do preparation like
 * 1. call ELF init functions
 */
extern "C" void premain(AppInitState* state_ptr) {
  for (int i = 0; i < state_ptr->init_fn_table_size; ++i) {
    state_ptr->init_fn_table[i]();
  }
}
