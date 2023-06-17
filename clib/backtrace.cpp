#include <assert.h>
#include <dwarf.h>

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

// the kernel elf file is loaded here
#define ELF_ADDR ((void*) 0x10000)
DwarfContext _dwarf_ctx((uint8_t*) ELF_ADDR, false);

DwarfContext& get_dwarf_ctx() {
  if (!_dwarf_ctx.initialized) {
    _dwarf_ctx.init();
  }
  return _dwarf_ctx;
}

/*
 * Only tested in kernel mode so far. TODO: test in user mode.
 */
void backtrace() {
  printf("backtrace\n");
  uint32_t ebp = asm_get_ebp(), eip = asm_get_eip();
  do {
    const FunctionEntry* func_entry = get_dwarf_ctx().find_func_entry_by_addr(eip);
    const LineNoEntry* lineno_entry = get_dwarf_ctx().find_lineno_entry_by_addr(eip);
    if (!lineno_entry) {
      printf("Fail to get lineno entry for addr 0x%x\n", eip);
      // don't call assert to avoid infinite recursion.
      safe_assert(false);
    }

    if (!func_entry) {
      printf("  ebp 0x%x, eip 0x%x func %s (%s:%d)\n", ebp, eip, "Unknown", lineno_entry->file_name, lineno_entry->lineno);
    } else {
      printf("  ebp 0x%x, eip 0x%x func %s (%s:%d)\n", ebp, eip, func_entry->name, lineno_entry->file_name, lineno_entry->lineno);
    }

    eip = ((uint32_t*) ebp)[1];
    ebp = *(uint32_t*) ebp;
  } while (ebp);
}
