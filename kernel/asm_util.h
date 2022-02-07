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

#ifdef __cplusplus
}
#endif

#endif
