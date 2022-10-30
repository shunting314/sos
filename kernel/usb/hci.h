#pragma once

#include <kernel/pci.h>
#include <kernel/paging.h>

class USBControllerDriver {
 public:
  explicit USBControllerDriver(const PCIFunction& pci_func) : pci_func_(pci_func) {
  }

  // XXX DON'T SUPPORT virtual functions yet! Enable this will cause undefined
  // symbols like: `vtable for __cxxabiv1::__si_class_type_info'
  // virtual void reset() = 0;
 protected:
  // assumes the only Bar available is a memory bar.
  // Use this for OHCI/EHCI/xHCI.
  //
  // UHCI is different since it uses an IO Bar.
  Bar setupMembar() {
    Bar membar;
    for (int i = 0; i < 6; ++i) {
      auto curbar = pci_func_.getBar(i);
      if (curbar) {
        assert(!membar);
        assert(curbar.isMem());
        membar = curbar;
      }
    }
    assert(membar);
    printf("MEMBar:\n");
    membar.print();
    map_region((phys_addr_t) kernel_page_dir, membar.get_addr(), membar.get_addr(), membar.get_size(), MAP_FLAG_WRITE);
    return membar;
  }
 protected:
  PCIFunction pci_func_;
};
