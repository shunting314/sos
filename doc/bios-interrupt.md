# BIOS Interrupt List

## int $0x10
### print a character
AH: 0xe
AL: character to print

### set video mode
AH: 0
AL: desired video mode

0x00: text mode. 40x25
0x03: text mode. 80x25
0x13: graphical mode.

Setting the video mode has the side effect of clearing the screen.

## int $0x13
### get drive geometry (refer to the osdev wiki)
AH: 8
DL: drive number

After returning
- DH contains "Number of Heads" - 1
- AND the value returend in CL with 0x3f to get the 'Sectors per Track'

### Check if LBA Extension is Supported
AH: 0x41
BX: 0x55aa
dl: drive number

THe carry flag will be set if extensions are not supported.

### Read Disk with LBA
AH: 0x42
DL: drive number
DS:SI: point to the 'Disk Address Packet' structure

After returning
- the carry flag will be set if there is any error during the transfer
- AH will be set to 0 on success

Format of the disk address packet
- 1 byte: size of packet. Should be hardcoded to 16 for this format.
- 1 byte: always 0
- 2 byte: number of sectors to transfer. There should be some cap. The safest way is to trasfer one sector at a time.
- 2 byte: offset for the transfer buffer
- 2 byte: segment for the transfer buffer
- 4 byte: lower 32 bits of 48-bit starting LBA
- 4 byte: upper 16 bites of 48-bit starting LBA (Q: the rest 16 bits are ignored?)

Note
- the disk address packet should be 4 byte aligned
- the transfer buffer should be 2 byte aligned

# Reference
- [BIOS Interrupts List](http://www.ablmcc.edu.hk/~scy/CIT/8086_bios_and_dos_interrupts.htm)
- [Disk Access using BIOS - osdev](https://wiki.osdev.org/Disk_access_using_the_BIOS_(INT_13h)): Using LBA to access disk with BIOS is super useful!
