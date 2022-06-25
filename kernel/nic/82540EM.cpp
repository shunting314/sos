#include <kernel/nic/82540EM.h>
#include <kernel/paging.h>
#include <assert.h>

/*
 * According to '82540EM SDM: 4.1 PCI Configuration', 82540EM BAR 0 (if exist)
 * is the memory register, BAR 2 (if exist) is the IO register.
 */
void NICDriver_82540EM::init() {
  assert(pci_func_);
  Bar membar = pci_func_.getBar(0);
  Bar iobar = pci_func_.getBar(2);

  printf("82540EM Bars:\n");
  membar.print();
  iobar.print();
  assert(membar && !iobar && "We can only handle membar for now and we assume iobar does not exist");

  membar_ = membar;

  // map the membar to kernel pgdir
  // TODO: the address is in quite high space. In theory the linear address region
  // mapped here may be conflict with user app. Here are a few alternatives
  // 1. reserve some linear address regions in kernel space permanetly for this
  // 2. reserve a few temp virtual pages in kernel. Map on demand and unmap after
  //    done.
  map_region((phys_addr_t) kernel_page_dir, membar.get_addr(), membar.get_addr(), 4096 /* should we map more? */, MAP_FLAG_WRITE /* kernel RW */);

  mac_addr_ = MACAddr(
    read_eeprom_word(EEPROM_OFF_ENADDR_0),
    read_eeprom_word(EEPROM_OFF_ENADDR_1),
    read_eeprom_word(EEPROM_OFF_ENADDR_2)
  );
}

uint32_t NICDriver_82540EM::read_nic_register(int off) {
  assert((off & 3) == 0);
  // take advantage of the fact that we setup identity map in NICDriver_82540EM::init
  auto addr = (uint32_t*) (membar_.get_addr() + off);
  return *addr;
}

void NICDriver_82540EM::write_nic_register(int off, uint32_t val) {
  assert((off & 3) == 0);
  // take advantage of the fact that we setup identity map in NICDriver_82540EM::init
  auto addr = (uint32_t*) (membar_.get_addr() + off);
  *addr = val;
}

// check '82540EM SDM: 5.3.1 Software Access' for how to access a word in EEPROM
uint16_t NICDriver_82540EM::read_eeprom_word(int woff) {
  /*
   * It's necessary to set eerd_cont.data to 0 in order to read the correct content
   * from EEPROM.
   * We can do that by 'EERD_struct eerd_cont = {0};' which init all fields to 0;
   * or more specifically set 'eerd_cont.data' to 0 by 'eerd_cont.data = 0;'.
   * The manual does not mention that we need set eerd_cont.data to 0 for reading
   * EEPROM though!
   */
  EERD_struct eerd_cont;
  eerd_cont.data = 0;
  eerd_cont.addr = woff;
  eerd_cont.start = 1;

  write_nic_register(REG_OFF_EERD, regStructToUint32(eerd_cont));
  while (true) {
    volatile uint32_t val = read_nic_register(REG_OFF_EERD);
    eerd_cont = uint32ToRegStruct<EERD_struct>(val);
    if (eerd_cont.done) {
      break;
    }
  }
  return eerd_cont.data;
}
