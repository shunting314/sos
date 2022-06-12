#include <kernel/paging.h>
#include <kernel/asm_util.h>
#include <kernel/phys_page.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct paging_entry {
  uint32_t present : 1;
  uint32_t r_w : 1;
  uint32_t u_s : 1;
  uint32_t others : 9;
  uint32_t phys_page_no : 20;
};
typedef struct paging_entry paging_entry_t;
static_assert(sizeof(paging_entry_t) == 4);

#define CR0_PG (1UL << 31)
#define CR0_WP (1UL << 16)
#define PAGE_OFF_MASK 0xFFF
#define PAGE_SIZE 4096
#define PAGING_ENTRIES_PER_PAGE ((PAGE_SIZE) / sizeof(paging_entry_t))

#define PDIDX(la) ((la >> 22) & 0x3FF)
#define PTIDX(la) ((la >> 12) & 0x3FF)
#define PGOFF(la) (la & 0xFFF)

#define GET_PGENTRY_PTR(tbl, idx) (paging_entry_t*) ((uint32_t) tbl + idx * 4)
#define GET_PDE_PTR(tbl, la) GET_PGENTRY_PTR(tbl, PDIDX(la))
#define GET_PTE_PTR(tbl, la) GET_PGENTRY_PTR(tbl, PTIDX(la))

void map_page(phys_addr_t page_dir, uint32_t la_start, uint32_t pa_start, int map_flags) {
  paging_entry_t* ppde = GET_PDE_PTR(page_dir, la_start);
  if (!ppde->present) {
    phys_addr_t newpg = alloc_phys_page();
    memset((void*) newpg, 0, 4096);
    ppde->present = 1;
    if (map_flags & MAP_FLAG_WRITE) {
      ppde->r_w = 1;
    }
    if (map_flags & MAP_FLAG_USER) {
      ppde->u_s = 1;
    }
    ppde->phys_page_no = (newpg >> 12);
  }

  phys_addr_t page_tbl = (ppde->phys_page_no << 12);
  paging_entry_t* ppte = GET_PTE_PTR(page_tbl, la_start);

  assert(!ppte->present && "page already mapped. Unmap first");
 
  ppte->present = 1;
  if (map_flags & MAP_FLAG_WRITE) {
    ppte->r_w = 1;
  }
  if (map_flags & MAP_FLAG_USER) {
    ppte->u_s = 1;
  }
  ppte->phys_page_no = (pa_start >> 12);
}

void map_region_alloc(phys_addr_t page_dir, uint32_t la_start, uint32_t size, int map_flags) {
  if (size <= 0) {
    return;
  }
  if (la_start & PAGE_OFF_MASK) {
    size += la_start & PAGE_OFF_MASK;
    la_start &= ~PAGE_OFF_MASK;
  }

  uint32_t off = 0;
  for (; off < size; off += PAGE_SIZE) {
    uint32_t physpg = alloc_phys_page();
    map_page(page_dir, la_start + off, physpg, map_flags);
  }
}

void map_region(phys_addr_t page_dir, uint32_t la_start, uint32_t pa_start, uint32_t size, int map_flags) {
  if (size <= 0) {
    return;
  }
  assert((la_start & PAGE_OFF_MASK) == 0);
  assert((pa_start & PAGE_OFF_MASK) == 0);

  uint32_t off = 0;
  for (; off < size; off += PAGE_SIZE) {
    map_page(page_dir, la_start + off, pa_start + off, map_flags);
  }
}

void setup_paging() {
  // NOTE: gdt is still in the range of [0x7c00, 0x7dff]
  // NOTE: not map [0, 4095] on purpose so deref NULL is invalid
  //
  // We setup identity map for all physical memory available. This enables
  // us accessing physical memories easily.
  // map_region((phys_addr_t)kernel_page_dir, 4096, 4096, (uint32_t) END - 4096, MAP_FLAG_WRITE); // user no access; kernel read/write
  assert(phys_mem_amount <= 0x40000000); // TODO we can only handle at most 1G memory for now
  map_region((phys_addr_t)kernel_page_dir, 4096, 4096, (uint32_t) phys_mem_amount - 4096, MAP_FLAG_WRITE); // user no access; kernel read/write
  asm_set_cr3((uint32_t)kernel_page_dir);
  asm_cr0_enable_flags(CR0_PG | CR0_WP);
  printf("Paging enabled.\n");
}

/*
 * Release the page dir and associated page table/frames for a user app.
 *
 * Setup identity map in setup_paging() for physical memory greatly ease the work to release a page
 * dir, all page tables used and all page frames. We can access the page dir
 * and page table directly via the physical addresses!
 *
 * NOTE: app page directory is a super set of kernel page directory. When freeing
 * paging structures, we should skip those for kernel. We do the filtering using
 * paging_entry.u_s flag.
 */
void release_pgdir(phys_addr_t pgdir) {
  auto pde_list = (paging_entry*) pgdir;
  for (int i = 0; i < PAGING_ENTRIES_PER_PAGE; ++i) {
    if (pde_list[i].present && pde_list[i].u_s) {
      auto pte_list = (paging_entry*) (pde_list[i].phys_page_no << 12);
      for (int j = 0; j < PAGING_ENTRIES_PER_PAGE; ++j) {
        if (pte_list[j].present && pte_list[j].u_s) {
          // release the page frame
          free_phys_page(pte_list[j].phys_page_no << 12);
        }
      }
      // release the page table
      free_phys_page(pde_list[i].phys_page_no << 12);
    }
  }
  // release page directory
  free_phys_page(pgdir);
}
