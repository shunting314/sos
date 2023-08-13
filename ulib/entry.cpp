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
  push %eax  # exit code
  call exit
  # no need to pop since we can not reach here
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
