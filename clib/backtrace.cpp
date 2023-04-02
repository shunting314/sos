#include <assert.h>

extern "C" uint32_t asm_get_ebp();
extern "C" uint32_t asm_get_eip();

asm(R"(
.global asm_get_ebp
asm_get_ebp:
  mov %ebp, %eax
  ret
)");

asm(R"(
.global asm_get_eip
asm_get_eip:
  mov (%esp), %eax
  ret
)");

/*
 * Only tested in kernel mode so far. TODO: test in user mode.
 */
void backtrace() {
  printf("backtrace\n");
  uint32_t ebp = asm_get_ebp(), eip = asm_get_eip();
  do {
    printf("  ebp 0x%x, eip 0x%x\n", ebp, eip);
    eip = ((uint32_t*) ebp)[1];
    ebp = *(uint32_t*) ebp;
  } while (ebp);
}
