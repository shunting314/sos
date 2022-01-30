#ifndef KERNEL_ASM_UTIL_H
#define KERNEL_ASM_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

// TODO use inline assembly so we can get rid of this file?
void asm_lidt();

#ifdef __cplusplus
}
#endif

#endif
