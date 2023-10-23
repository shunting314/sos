#include <kernel/page_fault.h>
#include <kernel/idt.h>
#include <assert.h>
#include <kernel/asm_util.h>
#include <kernel/user_process.h>
#include <kernel/phys_page.h>
#include <kernel/paging.h>
#include <string.h>
#include <stdlib.h>

/*
 * adjust the page mapping for COW
 */
void handle_cow(uint32_t la, paging_entry_t* ppte) {
  uint32_t phys_addr = (ppte->phys_page_no << 12);
  int refcount = PhysPageStat::getRefCountUser(phys_addr);
  assert(refcount > 0);
  if (refcount == 1) {
    // the last reference. simply change a few bits
    ppte->r_w = 1;
    ppte->cow = 0;

    asm_invlpg(la);
  } else {
    // there are more references

    // copy the content to a new physical page
    phys_addr_t newpg = alloc_phys_page();
    memmove((void*) newpg, (void*) phys_addr, 4096);

    // remove the reference to the old page
    PhysPageStat::decRefCountUser(phys_addr);

    // setup new mapping
    ppte->phys_page_no = (newpg >> 12);
    PhysPageStat::incRefCountUser(newpg);
    ppte->r_w = 1;
    ppte->cow = 0;

    asm_invlpg(la);
  }
}

void pfhandler(InterruptFrame* framePtr) {
  uint32_t fault_addr = asm_get_cr2();
  UserProcess* curProcess = UserProcess::current();
  int pid = -1;
  if (curProcess) {
    pid = curProcess->get_pid();
  }
  printf("PFHANDLER pid %d (%s), error code 0x%x"
         ", eip 0x%x, cr2 0x%x\n",
         pid, curProcess ? curProcess->name : "N/A",  framePtr->error_code, framePtr->eip, fault_addr);

  // cr3 should equals to the current process's page direcotry
  phys_addr_t pgdir = curProcess->getPgdir();
  assert(asm_get_cr3() == pgdir);
  paging_entry_t* ppte = get_pte_ptr(pgdir, fault_addr);

  if (ppte->present && !ppte->r_w && ppte->u_s && ppte->cow) {
    handle_cow(fault_addr, ppte);
    framePtr->returnFromInterrupt();
  }
  assert(false && "non recoverable page fault");
}
