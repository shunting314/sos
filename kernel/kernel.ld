ENTRY(entry)
OUTPUT_FORMAT(elf32-i386)

SECTIONS {
  . = 0x100000;
  .text : {
    *(.text);
  }
  .init_array : {
    /* refer to ld's default script obtained by running 'ld --verbose' */
    *(.init_array .ctors)
  }
  .data : {
    *(.data);
  }
  .bss : {
    *(.bss);
  }
  END = .;
}
