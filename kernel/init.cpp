#include <kernel/init.h>
#include <kernel/ide.h>
#include <elf.h>
#include <stdlib.h>

typedef void init_fn_t(void);
class RAIICls {
 public:
  RAIICls(const char *msg) : msg_(msg) {
    printf("Hello from kernel: %s\n", msg);
  }
 private:
  const char* msg_;
};

RAIICls first_message("first message");
RAIICls second_message("second message");

#define FIRST_KERNEL_SECTOR 1

// the returned ptr may not be the beginning of the buffer since the file offset
// may not sit at the sector boundary
void *load_from_disk(char *buf, int bufsize, int file_off, int nbytes_needed) {
  int remainder = file_off % SECTOR_SIZE;
  file_off -= remainder;
  nbytes_needed = ROUND_UP(nbytes_needed + remainder, SECTOR_SIZE);
  assert(nbytes_needed <= bufsize);

  auto dev = createMasterIDE();
  dev.read((uint8_t*) buf, file_off / SECTOR_SIZE + FIRST_KERNEL_SECTOR, nbytes_needed / SECTOR_SIZE);
  return buf + remainder;
}

void kernel_elf_init() {
  char buf_ehdr[512]; // one sector is enough
  // The kernel ELF can have around 100 sections, each section header has 40 bytes.
  // The total buffer size needed would be around 4000 + 512 bytes.
  // The 512 bytes is for rounding to sector boundary. 4096 bytes buffer size
  // may not be enough.
  char buf_shdrtable[4096 * 2];
  char buf_init_fn_table[4096];

  Elf32_Ehdr* ehdr = (Elf32_Ehdr*) load_from_disk(buf_ehdr, sizeof(buf_ehdr), 0, sizeof(Elf32_Ehdr));

  Elf32_Shdr* shdrtable = (Elf32_Shdr*) load_from_disk(buf_shdrtable, sizeof(buf_shdrtable), ehdr->e_shoff, ehdr->e_shnum * sizeof(Elf32_Shdr));

  for (Elf32_Shdr* shdr = shdrtable; shdr != shdrtable + ehdr->e_shnum; ++shdr) {
    switch (shdr->sh_type) {
    case SHT_INIT_ARRAY: {
      if (shdr->sh_entsize != 4) {
        printf("unexpected init array entry size: %d\n", shdr->sh_entsize);
        break;
      }
      if (shdr->sh_size < 0 || shdr->sh_size % 4 != 0) {
        printf("unexpected init array size: %d\n", shdr->sh_size);
        break;
      }

      int fn_table_size = shdr->sh_size / 4;
      init_fn_t** elf_init_fn_table = (init_fn_t**) load_from_disk(buf_init_fn_table, sizeof(buf_init_fn_table), shdr->sh_offset, shdr->sh_size);
      for (int i = 0; i < fn_table_size; ++i) {
        elf_init_fn_table[i]();
      }
      break;
    }
    }
  }
}
