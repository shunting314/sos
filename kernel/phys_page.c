#include <kernel/phys_page.h>
#include <assert.h>
#include <stdio.h>

extern char END[];

// TODO: hardcode to 10M for now. Should find a way to detect this automatically
uint32_t phys_mem_amount = 10 * 1024 * 1024; // unit is byte

struct physical_page {
  struct physical_page* next;
} *free_list_hdr;

void free_phys_page(phys_addr_t phys_addr) {
#ifdef DEBUG_PHYS_PAGE
  printf("FREE physical page %p\n", phys_addr);
#endif
  assert((phys_addr & 0xFFF) == 0);
  struct physical_page* ppg = (struct physical_page*) phys_addr;

  ppg->next = free_list_hdr;
  free_list_hdr = ppg;
}

phys_addr_t alloc_phys_page() {
  assert(free_list_hdr != NULL && "OOM!");
  struct physical_page* ppg = free_list_hdr;
  free_list_hdr = free_list_hdr->next;
#ifdef DEBUG_PHYS_PAGE
  printf("ALLOC physical page %p\n", ppg);
#endif
  return (uint32_t) ppg;
}

uint32_t num_avail_phys_pages() {
  uint32_t num_pages = 0;
  for (const struct physical_page* cur = free_list_hdr; cur; cur = cur->next) {
    ++num_pages;
  }
  return num_pages;
} 

void setup_phys_page_freelist() {
  assert(phys_mem_amount % 4096 == 0);
  uint32_t first_free_page = ((((uint32_t) END) + 0xFFF) & ~0xFFF);
  int nalloc = 0;
  for (uint32_t free_page = first_free_page; free_page + 0xFFF < phys_mem_amount; free_page += 4096) {
    free_phys_page(free_page);
    nalloc += 1;
  }
  assert(free_list_hdr != NULL);
  printf("%d physical pages available initially\n", nalloc);
}
