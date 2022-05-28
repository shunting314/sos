#ifndef INC_ELF_H
#define INC_ELF_H

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

typedef struct
{
  Elf32_Word  sh_name;    /* Section name (string tbl index) */
  Elf32_Word  sh_type;    /* Section type */
  Elf32_Word  sh_flags;   /* Section flags */
  Elf32_Addr  sh_addr;    /* Section virtual addr at execution */
  Elf32_Off sh_offset;    /* Section file offset */
  Elf32_Word  sh_size;    /* Section size in bytes */
  Elf32_Word  sh_link;    /* Link to another section */
  Elf32_Word  sh_info;    /* Additional section information */
  Elf32_Word  sh_addralign;   /* Section alignment */
  Elf32_Word  sh_entsize;   /* Entry size if section holds table */
} Elf32_Shdr;

#define SHT_INIT_ARRAY    14    /* Array of constructors */

#endif
