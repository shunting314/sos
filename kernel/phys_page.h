#ifndef _KERNEL_PHYS_PAGE_H
#define _KERNEL_PHYS_PAGE_H

#include <stdint.h>

uint32_t alloc_phys_page();
void setup_phys_page_freelist();

#endif
