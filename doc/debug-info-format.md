# Note
- Enabling -g cause kernel elf file size increasing from 172k to 439k. We should move debug info to separate files and put that in the filesystem. They can be loaded by the kernel later rather than being loaded by BIOS.

- gcc/g++ generate debug information in DWARF V5 format by default as I wrote this.

# Reference
- [stabs - wikipedia](https://en.wikipedia.org/wiki/Stabs): stabs encodes debug information with special entries in the symbol table. It has been superceded by DWARF.
- [DWARF - wikipedia](https://en.wikipedia.org/wiki/DWARF).
- [DWARF official site](https://dwarfstd.org/): redirect to web.archive.org automatically.
- [Introduction to the DWARF Debugging Format](https://web.archive.org/web/20220808162022/https://www.dwarfstd.org/doc/Debugging%20using%20DWARF-2012.pdf): it's a very awesome description of dwarf format by the chair of the dwarf standard committee.
- [DWARF5 Spec](https://dwarfstd.org/doc/DWARF5.pdf)
