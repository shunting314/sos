#include <kernel/phys_page.h>
#include <assert.h>
#include <stdio.h>

extern char END[];

// TODO: hardcode to 10M for now. Should find a way to detect this automatically
uint32_t phys_mem_amount = 10 * 1024 * 1024; // unit is byte

struct physical_page {
  struct physical_page* next;
} *free_list_hdr;

void free_phys_page(struct physical_page* ppg) {
  ppg->next = free_list_hdr;
  free_list_hdr = ppg;
}

uint32_t alloc_phys_page() {
  assert(free_list_hdr != NULL);
  struct physical_page* ppg = free_list_hdr;
  free_list_hdr = free_list_hdr->next;
  return (uint32_t) ppg;
}

void setup_phys_page_freelist() {
  assert(phys_mem_amount % 4096 == 0);
  uint32_t first_free_page = ((((uint32_t) END) + 0xFFF) & ~0xFFF);
  int nalloc = 0;
  for (uint32_t free_page = first_free_page; free_page + 0xFFF < phys_mem_amount; free_page += 4096) {
    struct physical_page* ppg = (struct physical_page*) free_page;
    free_phys_page(ppg);
    nalloc += 1;
  }
  assert(free_list_hdr != NULL);
  printf("%d physical pages available initially\n", nalloc);
}
