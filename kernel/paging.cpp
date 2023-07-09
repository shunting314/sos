#include <kernel/paging.h>
#include <kernel/asm_util.h>
#include <kernel/phys_page.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CR0_PG (1UL << 31)
#define CR0_WP (1UL << 16)
#define PAGE_OFF_MASK 0xFFF
#define PAGING_ENTRIES_PER_PAGE ((PAGE_SIZE) / sizeof(paging_entry_t))

#define PDIDX(la) ((la >> 22) & 0x3FF)
#define PTIDX(la) ((la >> 12) & 0x3FF)
#define PGOFF(la) (la & 0xFFF)
#define MAKE_LADDR(pdx, ptx, off) ((uint32_t) ((pdx << 22) | (ptx << 12) | off))

#define GET_PGENTRY_PTR(tbl, idx) (paging_entry_t*) ((uint32_t) tbl + idx * 4)
#define GET_PDE_PTR(tbl, la) GET_PGENTRY_PTR(tbl, PDIDX(la))
#define GET_PTE_PTR(tbl, la) GET_PGENTRY_PTR(tbl, PTIDX(la))

/*
 * Return the pointer to the PTE for the linear address in the page directory.
 * Assumes the page directory entry exists. The logic is quite similar to
 * map_page except we don't need to allocate a page table here.
 *
 * It's okay if la is not page aligned.
 */
paging_entry_t* get_pte_ptr(phys_addr_t page_dir, uint32_t la) {
  paging_entry_t* ppde = GET_PDE_PTR(page_dir, la);
  assert(ppde->present); // assume the presence of the page table
  phys_addr_t page_tbl = (ppde->phys_page_no << 12);
  paging_entry_t* ppte = GET_PTE_PTR(page_tbl, la);
  return ppte;
}

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
    // increase the reference
    PhysPageStat::incRefCountUser(pa_start);
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
  assert(phys_mem_amount % 4096 == 0);
  map_region((phys_addr_t)kernel_page_dir, 4096, 4096, (uint32_t) phys_mem_amount - 4096, MAP_FLAG_WRITE); // user no access; kernel read/write

  // setup the heap. Don't do COW for simplicity for now.
  // Allocate the 2MB after the end of phys_mem_amount as heap space.
  // Revisit if this becomes too small.
  map_region_alloc((phys_addr_t) kernel_page_dir, phys_mem_amount, (1 << 20) * 2, MAP_FLAG_WRITE);

  asm_set_cr3((uint32_t)kernel_page_dir);
  asm_cr0_enable_flags(CR0_PG | CR0_WP);
  printf("Paging enabled.\n");

  dump_pgdir((uint32_t) kernel_page_dir);
  debug_paging_for_addr((uint32_t) kernel_page_dir, 0xb8000);
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
          // release the page frame if refcount reaches 0 after decrementing
          PhysPageStat::decRefCountUser(pte_list[j].phys_page_no << 12); 
        }
      }
      // release the page table
      free_phys_page(pde_list[i].phys_page_no << 12);
    }
  }
  // release page directory
  free_phys_page(pgdir);
}

static void cow_clone_page(uint32_t la, paging_entry& parent_pte, paging_entry& child_pte) {
  assert(parent_pte.present);
  assert(parent_pte.u_s);
  assert(!child_pte.present);

  // Need the following special logic for writable page. Don't need special
  // handing for readonly or COW pages.
  //
  // So here is a summary of different scenarios
  // 1. the parent page is writable. Turn that into a COW page (readonly page with COW flag on)
  // 2. the parent page is a readonly/cow page. Keep it as a readonly/cow page.
  if (parent_pte.r_w) {
    parent_pte.r_w = 0;

    // set the flag so we know this readonly page is actually a COW page.
    // The page fault handler should copy the page content and setup a writable
    // mapping (unless it's the last user space reference to this page, in which
    // case the pfhandler can directly change the mapping to be writable without
    // allocating a new physical page).
    assert(!parent_pte.cow);
    parent_pte.cow = 1;

    asm_invlpg(la);
  }

  child_pte = parent_pte;
  // increment refcount
  PhysPageStat::incRefCountUser(parent_pte.phys_page_no << 12);
}

static void dumb_clone_page(paging_entry& parent_pte, paging_entry& child_pte) {
  auto new_pgframe = alloc_phys_page();
  memmove((void*) new_pgframe, (void*) (parent_pte.phys_page_no << 12), 4096);
  child_pte = parent_pte;
  child_pte.phys_page_no = (new_pgframe >> 12);

  // increment refcount
  PhysPageStat::incRefCountUser(new_pgframe);
}

phys_addr_t clone_address_space(phys_addr_t parent_pgdir, bool use_cow) {
  auto child_pgdir = alloc_phys_page();
  memmove((void*) child_pgdir, (void*) kernel_page_dir, 4096);

  // clone address space. Note handle kernel mapping properly
  auto parent_pde_list = (paging_entry*) parent_pgdir;
  auto child_pde_list = (paging_entry*) child_pgdir;
  for (int i = 0; i < PAGING_ENTRIES_PER_PAGE; ++i) {
    if (parent_pde_list[i].present && parent_pde_list[i].u_s) {
      auto new_pgtbl = alloc_phys_page();
      memset((void*) new_pgtbl, 0, 4096);
      child_pde_list[i] = parent_pde_list[i];
      child_pde_list[i].phys_page_no = (new_pgtbl >> 12);

      auto parent_pte_list = (paging_entry*) (parent_pde_list[i].phys_page_no << 12);
      auto child_pte_list = (paging_entry*) (child_pde_list[i].phys_page_no << 12);
      for (int j = 0; j < PAGING_ENTRIES_PER_PAGE; ++j) {
        if (parent_pte_list[j].present && parent_pte_list[j].u_s) {
          if (use_cow) {
            cow_clone_page(MAKE_LADDR(i, j, 0), parent_pte_list[j], child_pte_list[j]);
          } else {
            dumb_clone_page(parent_pte_list[j], child_pte_list[j]);
          }
        }
      }
    }
  }

  return child_pgdir;
}

// end == 0 means wrap around.
void dump_range(uint32_t start, uint32_t end, uint32_t flags) {
  const char* flagsmap[] = {
    "r", // kernel readable
    "rw", // kernel writable
    "ur", // user readable
    "urw" // user writeable
  };
  printf("  0x%x - 0x%x %s\n", start, end, flagsmap[flags]); 
}

void dump_pgdir(phys_addr_t pgdir) {
  printf("pgdir physaddr 0x%x\n", pgdir);

  uint32_t start = 1; // a non page aligned value represents non exist
  uint32_t end;
  uint32_t flags;

  paging_entry* pde_list = (paging_entry*) pgdir;
  for (int i = 0; i < PAGING_ENTRIES_PER_PAGE; ++i) {
    paging_entry* pde = &pde_list[i]; 
    if (!pde->present) {
      continue;
    }
    paging_entry* pte_list = (paging_entry*) (pde->phys_page_no << 12);
    for (int j = 0; j < PAGING_ENTRIES_PER_PAGE; ++j) {
      paging_entry* pte = &pte_list[j];
      if (!pte->present) {
        continue;
      }

      uint32_t pgaddr = MAKE_LADDR(i, j, 0);

      // merge if we can
      uint32_t pgflags = (pde->r_w & pte->r_w) | ((pde->u_s & pte->u_s) << 1);
      if ((start & 0xfff) == 0 && end == pgaddr && flags == pgflags) {
        end = pgaddr + PAGE_SIZE;
        continue;
      }

      // print the previous segment if exist
      if ((start & 0xfff) == 0) {
        dump_range(start, end, flags);
      }

      start = pgaddr;
      end = pgaddr + PAGE_SIZE;
      flags = pgflags;
    }
  }

  if ((start & 0xfff) == 0) {
    dump_range(start, end, flags);
  }
}

void debug_paging_for_addr(uint32_t pgdir, uint32_t laddr) {
  laddr = (laddr & ~0xFFF);
  uint32_t pdidx = PDIDX(laddr);
  uint32_t ptidx = PTIDX(laddr);
  paging_entry* pde_list = (paging_entry*) pgdir;
  paging_entry* pde = &pde_list[pdidx];
  printf("Bebug paging for laddr 0x%x\n", laddr);

  if (!pde->present) {
    printf("PDE not present\n");
    return;
  }
  paging_entry* pte_list = (paging_entry*) (pde->phys_page_no << 12);
  paging_entry* pte = &pte_list[ptidx];
  if (!pte->present) {
    printf("PTE not present\n");
    return;
  }
  printf("Mapping exists\n");
}
