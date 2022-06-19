#ifndef _KERNEL_PAGING_H
#define _KERNEL_PAGING_H

#include <stdint.h>
#include <kernel/phys_page.h> // need the definition of phys_addr_t

#ifdef __cplusplus
extern "C" {
#endif

extern char kernel_page_dir[];

void setup_paging();
void release_pgdir(phys_addr_t pgdir);
void map_region_alloc(phys_addr_t page_dir, uint32_t la_start, uint32_t size, int map_flags);
void map_region(phys_addr_t page_dir, uint32_t la_start, uint32_t pa_start, uint32_t size, int map_flags);

#define MAP_FLAG_WRITE (1UL << 0)
#define MAP_FLAG_USER (1UL << 1)

#ifdef __cplusplus
}
#endif

#endif
