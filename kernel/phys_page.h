#ifndef _KERNEL_PHYS_PAGE_H
#define _KERNEL_PHYS_PAGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t phys_addr_t;

phys_addr_t alloc_phys_page();
void free_phys_page(phys_addr_t phys_addr);
void setup_phys_page_freelist();
extern uint32_t phys_mem_amount;

uint32_t num_avail_phys_pages();

struct PhysPageStat;
// will be initialized to point to an array with one entry of PhysPage for each
// physical page.
extern PhysPageStat* phys_page_stats;

struct PhysPageStat {
 public:
  static void incRefCountUser(phys_addr_t addr) {
    ++(getPhysPageStat(addr)->refcount_user);
  }

  // decrement refcount_user. Release the page if refcount_user reaches 0
  // after decrementing.
  static void decRefCountUser(phys_addr_t addr) {
    auto& stat = *getPhysPageStat(addr);
    assert(stat.refcount_user > 0);
    if (--stat.refcount_user == 0) {
      free_phys_page(addr);
    }
  }

  static PhysPageStat* getPhysPageStat(phys_addr_t addr) {
    assert((addr & 0xFFF) == 0);
    uint32_t pgno = (addr >> 12);
    return &phys_page_stats[pgno];
  }

  static uint32_t getRefCountUser(phys_addr_t addr) {
    auto& stat = *getPhysPageStat(addr);
    return stat.refcount_user;
  }

  // Track the number of references from page table entry to this physical page
  // in user space.  The field will be used to implement COW fork.
  // We only care about refereces from user space since:
  // 1. when the refcount_user is one, a write to a COW page can be directly be
  //    converted to a writable page. No need to copy the page.
  // 2. when a process terminates, we can release a physical page only when its
  //    refcount_user is 0. Otherwise, the physical page is still used by other
  //    processes.
  uint32_t refcount_user = 0;
};

#ifdef __cplusplus
}
#endif

#endif
