# Mass Storage Device (MSD)
USB defines CommandBlockWrapper to wrap a command for MSD device. The command set is decided by the interface sub class code. Assume the interface sub class code is 0x06 which is for SCSI. The SCSI command set is defined out of USB spec. Need refer to SCSI spec to understand the command set.

# Reference
- [Host Controller Interface - wikipedia](https://en.wikipedia.org/wiki/Host_controller_interface_(USB,_Firewire))
- [USB: The Universal Serial Bus](https://www.amazon.com/USB-Universal-Serial-Bus-8/dp/1717425364): a book about USB
- [USB - osdev](https://wiki.osdev.org/Universal_Serial_Bus): an awesome doc about USB.
- [USB Mass Storage Class Devices - osdev](https://wiki.osdev.org/USB_Mass_Storage_Class_Devices): referenced from the USB osdev wiki. The page is too simple.
- [USB Mass Storage Class Specification Overview](https://www.usb.org/sites/default/files/Mass_Storage_Specification_Overview_v1.4_2-19-2010.pdf): it's a prerequisite for 'USB Mass Storage Class Bulk-Only Transport'.
- [USB Mass Storage Class Bulk-Only Transport](https://www.usb.org/sites/default/files/usbmassbulk_10.pdf): referenced from the 'USB Mass Storage Class Devices' osdev wiki.
- [USB1 spec](x): I don't find the spec for USB1
- [USB2 Spec](https://www.usb.org/document-library/usb-20-specification)
- [USB3 Spec](https://www.usb.org/document-library/usb-32-revision-11-june-2022): roughly read.
- [USB4 Spec](https://www.usb.org/document-library/usb4r-specification): seems like a draft.

## UHCI
- [UHCI - osdev](https://wiki.osdev.org/Universal_Host_Controller_Interface): this doc simply lists registers and data structures. Good for reference. But still need refer to the UHCI spec for details.
- [Intel UHCI Spec](ftp://ftp.netbsd.org/pub/NetBSD/misc/blymn/uhci11d.pdf)

## XHCI
- [xHCI - wikipedia](https://en.wikipedia.org/wiki/Extensible_Host_Controller_Interface)
- [xHCI - osdev](https://wiki.osdev.org/EXtensible_Host_Controller_Interface): this page mentions [xHCI implementation in HaiKu OS](https://github.com/haiku/haiku/blob/master/src/add-ons/kernel/busses/usb/xhci.cpp). Maybe worth check that when needed.
- [Intel xHCI Spec](https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/extensible-host-controler-interface-usb-xhci.pdf): It's referenced from 'xHCI - wikipedia'.
