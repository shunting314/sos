#ifndef KERNEL_ASM_UTIL_H
#define KERNEL_ASM_UTIL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void asm_sti() {
  asm volatile("sti");
}

uint32_t asm_get_cr3();
void asm_set_cr3(uint32_t phys_addr);
uint32_t asm_get_cr2();
void asm_return_from_interrupt(void *peip, uint16_t return_ds);
void asm_cr0_enable_flags(uint32_t flags);
void asm_enter_user_mode(uint32_t stack, uint32_t eip);
void asm_load_tr();
void asm_lidt();

// la does not have to be page aligned
void asm_invlpg(uint32_t la);

#ifdef __cplusplus
}
#endif

#endif
