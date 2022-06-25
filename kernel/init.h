#pragma once

/*
 * Since the bootloader has very stringent size limit, we handle kernel ELF
 * INIT_ARRAY section after entering kernel code rather than inside the
 * boot loader.
 */
void kernel_elf_init();
