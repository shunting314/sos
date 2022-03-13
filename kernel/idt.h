#ifndef KERNEL_IDT_H
#define KERNEL_IDT_H

#ifdef __cplusplus
extern "C" {
#endif

void setup_idt();

// TODO: make use of these definitions rather than use magical numbers in assembly
#define KERNEL_CODE_SEG 8
#define KERNEL_DATA_SEG 16
#define USER_CODE_SEG (24 | 3)
#define USER_DATA_SEG (32 | 3)

#ifdef __cplusplus
}
#endif

#endif
