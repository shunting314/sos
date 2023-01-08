# Test on My Toshiba Laptop
On Qemu, the bootloader can be loaded by BIOS and get run with or without a partition table. But on my toshiba laptop, if a partition table is missing, I get error message: "No bootable device". Looks like BIOS skips the device. A partition table is required in that case.

# Partition Table Entry Format
A partition table entry contains 16 bytes. It has the following fields in that order:
- Status - 1 byte
  - 0x00 for inactive; 0x80 for active
- Start CHS - 3 bytes
- Partition Type - 1 byte. Value of 0 cause failure to boot on my Toshiba laptop.
- End CHS - 3 bytes
- Start LBA - 4 bytes
- Num of sectors in partition - 4 bytes

# Reference
- [Master Boot Record - wikipedia](https://en.wikipedia.org/wiki/Master_boot_record)
- [Partition Type - wikipedia](https://en.wikipedia.org/wiki/Partition_type)
- [Master Boot Record - osdev](https://wiki.osdev.org/MBR_(x86))
- [Partition Table - osdev](https://wiki.osdev.org/Partition_Table)
- [GUID Partition Table - wikipedia](https://en.wikipedia.org/wiki/GUID_Partition_Table): According to the MBR wikipedia page, GUID Partition Table (GPT) supersedes MBR.
