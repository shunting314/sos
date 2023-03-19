#ifndef KERNEL_IDT_H
#define KERNEL_IDT_H

#include <stdint.h>
#include <kernel/asm_util.h>

#ifdef __cplusplus
extern "C" {
#endif

void setup_idt();

// TODO: make use of these definitions rather than use magical numbers in assembly
#define KERNEL_CODE_SEG 8
#define KERNEL_DATA_SEG 16
#define USER_CODE_SEG (24 | 3)
#define USER_DATA_SEG (32 | 3)

// NOTE: the order of fields here is reverse to the mental order:
// the field defined in upper position comes in lower address.
struct InterruptFrame {
  uint32_t edi;
  uint32_t esi;
  uint32_t ebp;
  uint32_t oesp; // don't be confused with esp. It's ignored in popa
  uint32_t ebx;
  uint32_t edx;
  uint32_t ecx;
  uint32_t eax;

  uint32_t error_code;
  uint32_t eip;
  uint16_t cs;
  uint16_t cs_padding;
  uint32_t eflags;

  // these 2 fields are only available if previlege level changes
  uint32_t esp; // don't be confused with oesp
  uint16_t ss;
  uint16_t ss_padding;

  void returnFromInterrupt() {
    // set to different value depending on if we are return to kernel mode
    // or user mode.
    uint16_t return_ds;
    if (cs == KERNEL_CODE_SEG) {
      return_ds = KERNEL_DATA_SEG;
    } else {
      return_ds = USER_DATA_SEG;
    }
    asm_return_from_interrupt(&edi, return_ds);
  }
};

static_assert(sizeof(InterruptFrame) == 24 + 32);

int64_t getTick();

#ifdef __cplusplus
}
#endif

#endif
