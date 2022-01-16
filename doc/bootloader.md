# Hello-World bootloader That Prints a Message
Check `boot/hello.s`.

# Load Kernel from Disk
We can use [int13h/ah=2](http://www.ablmcc.edu.hk/~scy/CIT/8086_bios_and_dos_interrupts.htm#int13h_02h) BIOS interrupt to load sectors from the disk.

DL register needs to be set to the identifier of the disk that we want to load data from. According to the comment [here](https://github.com/cfenollosa/os-tutorial/blob/master/07-bootsector-disk/boot_sect_disk.asm#L15):

- 0 for the first floppy disk
- 1 for the second floppy disk etc
- 0x80 for the first hard disk
- 0x81 for the second hard disk etc.

We need specify the first sector to read data from. To specify a sector, we need provide a tuple of (cylinder number, head number, sector number). Note that sector number starts from 1 while cyliner number and head number start from 0.

# Enter Protected Mode and load kernel
Check comment in boot/bootloader.s for the procedure.

# Tools
## objdump
Use `objdump -d` to disassemble generated objective file. objdump accepts an option '--reloc' to also print relocation entries interleaved with the disassembled code.

## objcopy/readelf
Unfortunately I don't find a replacement for objcopy/readelf on MacOS. On Linux, we can use objcopy to convert an ELF file to the raw binary format. Now, I have to use a workaround to create the raw binary format file based on the objdump output. This is what boot/obj2bl.py does.

# Reference
- [BIOS Interrupts List](http://www.ablmcc.edu.hk/~scy/CIT/8086_bios_and_dos_interrupts.htm)
