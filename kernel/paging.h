#ifndef _KERNEL_PAGING_H
#define _KERNEL_PAGING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern char kernel_page_dir[];
typedef uint32_t phys_addr_t;

void setup_paging();
void map_region_alloc(phys_addr_t page_dir, uint32_t la_start, uint32_t size, int map_flags);

#define MAP_FLAG_WRITE (1UL << 0)
#define MAP_FLAG_USER (1UL << 1)

#ifdef __cplusplus
}
#endif

#endif
