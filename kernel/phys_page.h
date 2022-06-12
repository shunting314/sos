#ifndef _KERNEL_PHYS_PAGE_H
#define _KERNEL_PHYS_PAGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t phys_addr_t;

uint32_t alloc_phys_page();
void free_phys_page(phys_addr_t phys_addr);
void setup_phys_page_freelist();
extern uint32_t phys_mem_amount;

uint32_t num_avail_phys_pages();

#ifdef __cplusplus
}
#endif

#endif
