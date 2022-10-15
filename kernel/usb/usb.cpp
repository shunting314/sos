#include <kernel/usb/usb.h>
#include <kernel/usb/uhci.h>
#include <kernel/usb/usb_proto.h>
#include <kernel/usb/usb_device.h>
#include <kernel/usb/msd.h>

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

  ConfigurationDescriptor config_desc = dev.getConfigurationDescriptor();
  assert(config_desc.bLength == sizeof(ConfigurationDescriptor));
  assert(config_desc.bDescriptorType == (int) DescriptorType::CONFIGURATION);
  assert(config_desc.wTotalLength > config_desc.bLength);
  assert(config_desc.bNumInterfaces > 0);
  assert(config_desc.bConfigurationValue > 0);

  dev.setConfiguration(config_desc.bConfigurationValue);
  // verify that the device is in a configured state, i.e., the config value
  // is > 0
  assert(dev.getConfiguration() == config_desc.bConfigurationValue);
  printf("The device is configured with config value %d\n", config_desc.bConfigurationValue);

  dev.collectEndpointDescriptors();
}

void usb_init() {
  setup_uhci();
}
