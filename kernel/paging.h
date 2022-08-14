#ifndef _KERNEL_PAGING_H
#define _KERNEL_PAGING_H

#include <stdint.h>
#include <kernel/phys_page.h> // need the definition of phys_addr_t

#ifdef __cplusplus
extern "C" {
#endif

struct paging_entry {
  uint32_t present : 1;
  uint32_t r_w : 1;
  uint32_t u_s : 1;
  uint32_t others : 2;
  uint32_t accessed : 1;

  // it's an ignored/avl bits for page directory entry
  uint32_t dirty : 1;

  uint32_t others_2 : 2;
  // the following 3 bits are ignored by hardware and can be used by the
  // software. They are also called AVL (available) bits sometimes.
  uint32_t cow : 1;
  uint32_t two_more_ignored_bits : 2;
  uint32_t phys_page_no : 20;
};
typedef struct paging_entry paging_entry_t;
static_assert(sizeof(paging_entry_t) == 4);

extern char kernel_page_dir[];

paging_entry_t* get_pte_ptr(phys_addr_t page_dir, uint32_t la);
void setup_paging();
void release_pgdir(phys_addr_t pgdir);
phys_addr_t clone_address_space(phys_addr_t parent_pgdir, bool use_cow);
void map_region_alloc(phys_addr_t page_dir, uint32_t la_start, uint32_t size, int map_flags);
void map_region(phys_addr_t page_dir, uint32_t la_start, uint32_t pa_start, uint32_t size, int map_flags);

#define MAP_FLAG_WRITE (1UL << 0)
#define MAP_FLAG_USER (1UL << 1)

#define PAGE_SIZE 4096

#ifdef __cplusplus
}
#endif

#endif
