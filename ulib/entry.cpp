#include <app_init_state.h>
#include <stdio.h>

// TODO support global destructor
asm(R"(
.global entry
entry:
  push %esp
  call premain
  add $4, %esp

  # maybe linker has special calling conventain for main.
  # print &argc will show 1!
  # Use a function named mymain resolves that.
  call mymain
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
