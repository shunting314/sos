#include <kernel/usb/usb.h>
#include <kernel/usb/uhci.h>

PCIFunction uhci_func, ohci_func, ehci_func, xhci_func;
UHCIDriver uhci_driver;

void setup_uhci() {
  assert(uhci_func);
  uhci_driver = UHCIDriver(uhci_func);
  uhci_driver.reset();
}

void usb_init() {
  setup_uhci();
}
