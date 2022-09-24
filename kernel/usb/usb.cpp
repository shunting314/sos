#include <kernel/usb/usb.h>
#include <kernel/usb/uhci.h>
#include <kernel/usb/usb_proto.h>

// for usb initialization
PCIFunction uhci_func, ohci_func, ehci_func, xhci_func;
UHCIDriver uhci_driver;

void setup_uhci() {
  assert(uhci_func);
  uhci_driver = UHCIDriver(uhci_func);
  uhci_driver.reset();

  DeviceDescriptor partial_device_desc = uhci_driver.getDeviceDescriptor(/* use_default_addr */ true);
  assert(partial_device_desc.bLength == 18);
}

void usb_init() {
  setup_uhci();
}
