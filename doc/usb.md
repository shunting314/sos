# Mass Storage Device (MSD)
USB defines CommandBlockWrapper to wrap a command for MSD device. The command set is decided by the interface sub class code. Assume the interface sub class code is 0x06 which is for SCSI. The SCSI command set is defined out of USB spec. Need refer to SCSI spec to understand the command set.

# XHCI Ring Management
Q: how to avoid the producer from overflowing the ring?
A: For the command/transfer ring, the host is the producer. The host maintains the enqueue pointer while the controller maintains the dequeue pointer. Driver code should maintain a shadow copy of the dequeue pointer in the controller. Whenever the driver get an event trb with a successful code, the driver should advance it's shadow copy of the dequeue pointer. It's safe for the host to produce more items as long as the enqueue pointer does not catch up with the shadow copy of the dequeue pointer.

  For the event ring, the host is the consumer. Whenever the host consumes an item from the ring, the host may tell the controller the new position of the dequeue pointer by setting some controler registers.

Q: how to avoid the consumser from underflowing the ring?
  The consumer can consume an item only if the item's cycle bit matches the consumer's cycle state (CCS). When the ring is empty, it's guaranteed that the item's cycle bit does not match CCS.

# Reference
- [Host Controller Interface - wikipedia](https://en.wikipedia.org/wiki/Host_controller_interface_(USB,_Firewire))
- [USB: The Universal Serial Bus](https://www.amazon.com/USB-Universal-Serial-Bus-8/dp/1717425364): a book about USB
- [USB - osdev](https://wiki.osdev.org/Universal_Serial_Bus): an awesome doc about USB.
- [USB - wikipedia](https://en.wikipedia.org/wiki/USB):
  - The wiki has a pointer to the Intel UHCI spec.
  - The USB bridge cables mentioned in the wiki that can be used to trasfer data between 2 computers is very interesting! But it seems to be deprecated by 'dual-role USB connections'. Actually, this kind of usage of USB is not surprising considering connecting a mobile phone to a desktop/laptop via USB. This connection can be used to charge the mobile phone or trasfer data.
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
- [Intel xHCI Spec](https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/extensible-host-controler-interface-usb-xhci.pdf): It's referenced from 'xHCI - wikipedia'. The most important port is how to manage the command/transfer/event rings.
