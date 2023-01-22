#include <kernel/usb/usb.h>
#include <kernel/usb/uhci.h>
#include <kernel/usb/xhci.h>
#include <kernel/usb/usb_proto.h>
#include <kernel/usb/usb_device.h>
#include <kernel/usb/msd.h>

// for usb initialization
PCIFunction uhci_func, ohci_func, ehci_func, xhci_func;
UHCIDriver uhci_driver;
XHCIDriver xhci_driver;

void setup_uhci() {
  assert(uhci_func);
  uhci_driver = UHCIDriver(uhci_func);
  uhci_driver.reset();

  // TODO: we should not assume a mass storage device but detect the device type.
  MassStorageDevice dev(&uhci_driver);
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
  dev.readCapacity();

  // read a block from MSD
  uint8_t blockData[512];
  assert(sizeof(blockData) >= dev.blockSize());
  dev.readBlocks(0, 1, blockData);
  printf("Data from USB:\n");
  for (int i = 0; i < dev.blockSize(); ++i) {
    putchar((char) blockData[i]);
  }
  printf("\n");

  // write a block to MSD. Disable by default to avoid mutate the media
  #if 0
  uint8_t sendData[512];
  assert(sizeof(sendData) >= dev.blockSize());
  for (int i = 0; i < dev.blockSize(); ++i) {
    sendData[i] = (i % 26 + 'a');
  }
  dev.writeBlocks(0, 1, sendData);
  printf("Done writing a block to MSD\n");
  #endif
}

// TODO avoid using this global variable
MassStorageDevice<XHCIDriver> msd_dev;

void setup_xhci() {
  assert(xhci_func);

  xhci_driver = XHCIDriver(xhci_func);
  xhci_driver.reset();

  // TODO: we should not assume a mass storage device but detect the device type.
  msd_dev = MassStorageDevice(&xhci_driver);
  xhci_driver.initializeDevice(&msd_dev);

  msd_dev.readCapacity();
  // read a block from MSD
  uint8_t blockData[512];
  assert(sizeof(blockData) >= msd_dev.blockSize());

  #if 1
  msd_dev.readBlocks(0, 1, blockData);
  #else
  // trigger ring wrap around
  for (int iter = 0; iter < 512; ++iter) {
    printf("Read iter %d\n", iter);
    msd_dev.readBlocks(0, 1, blockData);
  }
  #endif

  #if USB_BOOT
  // in usb boot, the first sector of the USB drive is the MBR, do hex dump
  // dump the first 2 and the last 2 paragraphs
  hexdump(blockData, 32);
  printf("...\n");
  hexdump(blockData + msd_dev.blockSize() - 32, 32);
  #else
  printf("Data from USB:\n");
  for (int i = 0; i < msd_dev.blockSize(); ++i) {
    putchar((char) blockData[i]);
  }
  printf("\n");
  #endif

  // write a block to MSD. Disable by default to avoid mutate the media
  #if 0
  uint8_t sendData[512];
  assert(sizeof(sendData) >= msd_dev.blockSize());
  for (int i = 0; i < msd_dev.blockSize(); ++i) {
    sendData[i] = (i % 26 + 'a');
  }
  msd_dev.writeBlocks(0, 1, sendData);
  printf("Done writing a block to MSD\n");
  #endif
}

void usb_init() {
  #if 0
  setup_uhci();
  #endif
  #if 1
  setup_xhci();
  #endif
}
