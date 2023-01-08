#include <stdint.h>
#include <elf.h>

void load_kernel_and_enter() {
  void* elf_file_start = (void *) 0x10000;
  Elf32_Ehdr *elf_hdr = (Elf32_Ehdr *) elf_file_start;

  if (elf_hdr->e_ident[0] != 0x7f || 
      elf_hdr->e_ident[1] != 'E' ||
      elf_hdr->e_ident[2] != 'L' ||
      elf_hdr->e_ident[3] != 'F') {
    goto fail;
  }

  Elf32_Phdr* phdrtable = elf_file_start + elf_hdr->e_phoff;
  for (Elf32_Phdr* phdr = phdrtable; phdr != phdrtable + elf_hdr->e_phnum; ++phdr) {
    // loadable segment
    if (phdr->p_type == PT_LOAD) {
      // memcpy
      char *src_addr = elf_file_start + phdr->p_offset;
      char *dst_addr = (void *) phdr->p_vaddr;
      char *src_addr_end = elf_file_start + phdr->p_offset + phdr->p_filesz;
      for (; src_addr != src_addr_end; ++src_addr, ++dst_addr) {
        *dst_addr = *src_addr;
      }

      // memset
      for (int i = phdr->p_filesz; i < phdr->p_memsz; ++i, ++dst_addr) {
        *dst_addr = 0;
      }
    }
  }

  // enter kernel
  ((void(*)())elf_hdr->e_entry)();

fail:
  while (1) {
  }
}
