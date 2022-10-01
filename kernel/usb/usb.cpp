#include <kernel/usb/usb.h>
#include <kernel/usb/uhci.h>
#include <kernel/usb/usb_proto.h>
#include <kernel/usb/usb_device.h>

// for usb initialization
PCIFunction uhci_func, ohci_func, ehci_func, xhci_func;
UHCIDriver uhci_driver;

void setup_uhci() {
  assert(uhci_func);
  uhci_driver = UHCIDriver(uhci_func);
  uhci_driver.reset();

  USBDevice dev(&uhci_driver);
  DeviceDescriptor device_desc_0 = dev.getDeviceDescriptor();
  assert(device_desc_0.bLength == 18);
  printf("Endpoint 0 max packet size %d\n", device_desc_0.bMaxPacketSize0);
  printf("Num config %d\n", device_desc_0.bNumConfigurations);
  assert(device_desc_0.bNumConfigurations > 0);
  dev.assignAddress();
  // read the device descriptor again with the assigned address
  DeviceDescriptor device_desc = dev.getDeviceDescriptor();
  assert(device_desc.bLength == 18);
  assert(device_desc.bMaxPacketSize0 == device_desc_0.bMaxPacketSize0);
  assert(device_desc.bNumConfigurations == device_desc_0.bNumConfigurations);
}

void usb_init() {
  setup_uhci();
}
