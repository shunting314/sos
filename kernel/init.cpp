#include <kernel/init.h>
#include <elf.h>

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

void kernel_elf_init() {
  void* elf_cont = (void *) 0x10000;
  Elf32_Ehdr *ehdr = (Elf32_Ehdr *) elf_cont;

  Elf32_Shdr* shdrtable = (Elf32_Shdr*) (elf_cont + ehdr->e_shoff);
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
      init_fn_t** elf_init_fn_table = (init_fn_t**) (elf_cont + shdr->sh_offset);
      for (int i = 0; i < fn_table_size; ++i) {
        elf_init_fn_table[i]();
      }
      break;
    }
    }
  }
}
