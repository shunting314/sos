#include <kernel/pci.h>

static void lsdevice(uint32_t bus_id, uint32_t dev_id) {
  PCIFunction func0(bus_id, dev_id, 0);

  // the device should not have any functions if function 0 does not exist
  if (!func0) {
    return;
  }

  func0.dump();

  auto header_type = func0.header_type();
  // the most significant bit of header_type is 1 for multi-function device
  if (header_type & 0x80) {
    for (uint32_t func_id = 1; func_id < MAX_NUM_FUNCTIONS_PER_DEVICE; ++func_id) {
      PCIFunction func_other(bus_id, dev_id, func_id);
      if (func_other) {
        func_other.dump();
      }
    }
  }
}

/*
 * TODO: this method enumerate thru all possible bus id and device id.
 * A more efficient method is do a DFS search. Check PCI osdev wiki.
 */
void lspci() {
  uint32_t bus_id;
  uint32_t dev_id;
  for (bus_id = 0; bus_id < MAX_NUM_BUS; ++bus_id) {
    for (dev_id = 0; dev_id < MAX_NUM_DEVICES_PER_BUS; ++dev_id) {
      lsdevice(bus_id, dev_id);
    }
  }
}
