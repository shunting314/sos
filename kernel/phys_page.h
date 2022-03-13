#ifndef _KERNEL_PHYS_PAGE_H
#define _KERNEL_PHYS_PAGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t alloc_phys_page();
void setup_phys_page_freelist();
extern uint32_t phys_mem_amount;

#ifdef __cplusplus
}
#endif

#endif
