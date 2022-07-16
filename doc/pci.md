# PCI Command & Status
In PCI configuration space, there is a 32 bits `command_status` register at offset 4. The low 3 bits of the command register worth noting:

- I/O Space (bit 0): enable this bit so CPU can access the device's I/O space
- Memory Space (bit 1): enable this bit so CPU can access the device's memory space
- Bus Master (bit 2): enable this bit so the device can do DMA.

# Reference
- [PCI Basics and Bus Enumeration - youtube](https://www.youtube.com/watch?v=qhIHu8mFrdg): this video explains the bus enumeration process pretty clearly.
- [PCI - wikipedia](https://en.wikipedia.org/wiki/Peripheral_Component_Interconnect): Talk too much about circuits.
- [PCI- osdev](https://wiki.osdev.org/PCI)
  - There are 2 fixed ports for PCI: 0xCF8 `CONFIG_ADDREES`, 0xCFC `CONFIG_DATA`. A configuration register access transction includes 2 steps: write the address encoding bus number, device number, function number and configuration register index to the `CONFIG_ADDRESS` port; then read or write a 4 bytes word from the `CONFIG_DATA` port.
  - The PCI configuration registers must be accessed as a whole word (4 bytes). Software should do necessary bit operations if only partial of a word is needed. This implies that the lowest 2 bits of the address written to the `CONFIG_ADDRESS` port must be 0.
  - PCI devices are inherently little-endian. big-endian processors like Power PC need perform the proper byte-swapping of data read from or written to the PCI device.
  - Vendor id 0xFFFF is invalid. Hardware returns all 1 bits for reading configuration registers of non-exist functions. Checking the returned vendor id against 0xFFFF can be used to detect if the function exists. PCI-SIG maintains the list of registered vendor ids [here](https://pcisig.com/membership/member-companies).
  - NOTE: it's necessary to check the highest bit of header type byte for function 0 to determine if the current device is a multi-function device. Some single-function devices may report details for 'function 0' for every function.
- [PCI configuration space - wikipedia](https://en.wikipedia.org/wiki/PCI_configuration_space)
- [How is a PCI/PCIe BAR size determined - StackOverflow](https://stackoverflow.com/questions/19006632/how-is-a-pci-pcie-bar-size-determined)
  - the answer assumes memory space BAR since the calculation masks the lowest 4 bits (IO space BAR need mask the lowest 2 bits).
- [lspci - wikipedia](https://en.wikipedia.org/wiki/Lspci)
- [Official Website for PCI and PCIe](https://pcisig.com/) # PCI wiki links to here
- [PCI-X - wikipedia](https://en.wikipedia.org/wiki/PCI-X): PCI-X tries to improve upon PCI but like PCI, PCI-X is also superseded by PCIe.
- [PCI Express - wikipedia](https://en.wikipedia.org/wiki/PCI_Express)
- [PCI Express - osdev](https://wiki.osdev.org/PCI_Express) NOTE: the page is WIP.
- [Bus Mastering - wikipedia](https://en.wikipedia.org/wiki/Bus_mastering)
