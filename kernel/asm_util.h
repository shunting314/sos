#ifndef KERNEL_ASM_UTIL_H
#define KERNEL_ASM_UTIL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// TODO use inline assembly so we can get rid of this file?
void asm_lidt();
void asm_sti();
void asm_return_from_interrupt(void *peip);
void asm_outb(uint16_t port, uint8_t data);
uint8_t asm_inb(uint16_t port);
void asm_outw(uint16_t port, uint16_t data);
uint16_t asm_inw(uint16_t port);
void asm_set_cr3(uint32_t phys_addr);
void asm_cr0_enable_flags(uint32_t flags);
void asm_enter_user_mode(uint32_t stack, uint32_t eip);
void asm_load_tr();

#ifdef __cplusplus
}
#endif

#endif
