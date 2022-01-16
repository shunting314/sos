typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

/* ELF data structures are copied from glibc elf/elf.h */

/* Type for a 16-bit quantity.  */
typedef uint16_t Elf32_Half;

/* Types for signed and unsigned 32-bit quantities.  */
typedef uint32_t Elf32_Word;
typedef int32_t  Elf32_Sword;

/* Types for signed and unsigned 64-bit quantities.  */
typedef uint64_t Elf32_Xword;
typedef int64_t  Elf32_Sxword;

/* Type of addresses.  */
typedef uint32_t Elf32_Addr;

/* Type of file offsets.  */
typedef uint32_t Elf32_Off;

#define EI_NIDENT (16)

typedef struct
{
  unsigned char e_ident[EI_NIDENT]; /* Magic number and other info */
  Elf32_Half  e_type;     /* Object file type */
  Elf32_Half  e_machine;    /* Architecture */
  Elf32_Word  e_version;    /* Object file version */
  Elf32_Addr  e_entry;    /* Entry point virtual address */
  Elf32_Off e_phoff;    /* Program header table file offset */
  Elf32_Off e_shoff;    /* Section header table file offset */
  Elf32_Word  e_flags;    /* Processor-specific flags */
  Elf32_Half  e_ehsize;   /* ELF header size in bytes */
  Elf32_Half  e_phentsize;    /* Program header table entry size */
  Elf32_Half  e_phnum;    /* Program header table entry count */
  Elf32_Half  e_shentsize;    /* Section header table entry size */
  Elf32_Half  e_shnum;    /* Section header table entry count */
  Elf32_Half  e_shstrndx;   /* Section header string table index */
} Elf32_Ehdr;

typedef struct
{
  Elf32_Word  p_type;     /* Segment type */
  Elf32_Off p_offset;   /* Segment file offset */
  Elf32_Addr  p_vaddr;    /* Segment virtual address */
  Elf32_Addr  p_paddr;    /* Segment physical address */
  Elf32_Word  p_filesz;   /* Segment size in file */
  Elf32_Word  p_memsz;    /* Segment size in memory */
  Elf32_Word  p_flags;    /* Segment flags */
  Elf32_Word  p_align;    /* Segment alignment */
} Elf32_Phdr;

#define PT_LOAD   1   /* Loadable program segment */

// the build system for the bootloader is quite simple and only support an
// empty .data/.bss for now
// void *elf_file_start = (void *) 0x10000;

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
      char *dst_addr = phdr->p_vaddr;
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
