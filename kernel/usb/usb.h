#pragma once

#include <kernel/pci.h>

// for usb initialization
extern PCIFunction uhci_func;
extern PCIFunction ohci_func;
extern PCIFunction ehci_func;
extern PCIFunction xhci_func;

void usb_init();
