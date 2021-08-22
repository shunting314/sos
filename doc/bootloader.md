# Hello-World bootloader That Prints a Message

# Tools
## objdump
Use `objdump -d` to disassemble generated objective file. objdump accepts an option '--reloc' to also print relocation entries interleaved with the disassembled code.

## objcopy/readelf
Unfortunately I don't find a replacement for objcopy/readelf on MacOS. On Linux, we can use objcopy to convert an ELF file to the raw binary format. Now, I have to use a workaround to create the raw binary format file based on the objdump output. This is what boot/obj2bl.py does.

# Reference
- [BIOS Interrupts List](http://www.ablmcc.edu.hk/~scy/CIT/8086_bios_and_dos_interrupts.htm)
