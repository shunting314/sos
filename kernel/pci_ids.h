#pragma once
/* This file contains various PCI ids such as vendor ids and devices ids for each vendor. */

// since PCI config space is litte endian, it will shows as 86 80 in the hex dump.
#define PCI_VENDOR_INTEL 0x8086

#define PCI_DEVICE_INTEL_PANTHER_POINT_XHCI 0x1e31

#define PCI_VENDOR_REALTEK 0x10ec
#define PCI_DEVICE_RTL88EE 0x8179

/*
 * Each PCI function has 256 bytes configuration space. The first 64 bytes are
 * standadized. The remainder are available for vendor-defined purposes.
 */
// Follow the definitions in haiku OS
// https://github.com/haiku/haiku/blob/9d51367f16a37bf8a309f3b7feb31a8d1d8c89ee/src/add-ons/kernel/busses/usb/xhci.cpp#L590
#define PCI_CONFIG_INTEL_USB3PRM 0xdc // USB 3.0 Port Routing Mask
#define PCI_CONFIG_INTEL_USB3_PSSEN 0xd8 // USB 3.0 Port SuperSpeed Enable
#define PCI_CONFIG_INTEL_USB2PRM 0xd4 // USB 2.0 Port Routing Mask
#define PCI_CONFIG_INTEL_XUSB2PR 0xd0 // USB 2.0 Port Routing
