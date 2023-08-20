#include <kernel/loader.h>
#include <kernel/simfs.h>
#include <kernel/user_process.h>
#include <kernel/phys_page.h>
#include <kernel/paging.h>
#include <elf.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <app_init_state.h>

// TODO: this trivial implementation only handle ELF file less than around 1000x BLOCK_SIZE!
// TODO: dynamically allocate the memory using kmalloc
// TODO: not reentrant
uint8_t launch_buf[BLOCK_SIZE * 10 + BLOCK_SIZE * 1024];

/*
 * If should_resume is true, the function will resume the child process right away
 * and not return;
 * Otherwise the function returns the child process id on success.
 */
int launch(const char* path, bool should_resume) {
  auto dent = SimFs::get().walkPath((char*) path);
  if (!dent) {
    printf("Path does not exist %s\n", path);
    return -1;
  }
  if (dent.file_size < sizeof(Elf32_Ehdr)) {
    printf("Invalid ELF file: file size smaller than ELF file header size\n");
    return -1;
  }
  assert(dent.file_size < sizeof(launch_buf) / sizeof(*launch_buf));
  for (int i = 0; i < dent.file_size; i += BLOCK_SIZE) {
    uint32_t logical_blkid = i / BLOCK_SIZE;
    uint32_t phys_blkid = dent.logicalToPhysBlockId(logical_blkid);
    SimFs::get().readBlock(phys_blkid, &launch_buf[i]);
  }

  UserProcess* proc = UserProcess::load(launch_buf);
  if (!proc) {
    printf("Fail to create process\n");
    return -1;
  }
  if (should_resume) {
    proc->resume();
  }
  return proc->get_pid();
}

/*
 * Push entry to app stack and decrement app esp.
 */
template <typename T>
static void push_to_app_stack(uint32_t& app_esp, const T& entry) {
  int entry_size = ROUND_UP(sizeof(entry), 4);
  app_esp -= entry_size;
  T* dst_entry = (T*) app_esp;
  *dst_entry = entry;
}

/*
 * Setup AppInitState which contains initialization information kernel pass to app.
 */
uint32_t setup_app_init_state(Elf32_Ehdr* ehdr) {
  uint8_t* elf_cont = (uint8_t*) ehdr;
  uint32_t app_esp = user_process_va_start;
  AppInitState app_init_state;

  Elf32_Shdr* shdrtable = (Elf32_Shdr*) (elf_cont + ehdr->e_shoff);
  bool found_init_array = false;
  for (Elf32_Shdr* shdr = shdrtable; shdr != shdrtable + ehdr->e_shnum; ++shdr) {
    switch (shdr->sh_type) {
    case SHT_INIT_ARRAY:
      if (found_init_array) {
        printf("Ignore duplicate init_array section\n");
        break;
      }
      found_init_array = true;
      if (shdr->sh_entsize != 4) {
        printf("unexpected init array entry size: %d\n", shdr->sh_entsize);
        break;
      }
      if (shdr->sh_size < 0 || shdr->sh_size % 4 != 0) {
        printf("unexpected init array size: %d\n", shdr->sh_size);
        break;
      }

      app_init_state.init_fn_table_size = shdr->sh_size / 4;
    
      init_fn_t** elf_init_fn_table = (init_fn_t**) (elf_cont + shdr->sh_offset);
      // push in reverse order so the array in stack are in the correct order.
      for (int i = app_init_state.init_fn_table_size - 1; i >= 0; --i) {
        auto init_fn = elf_init_fn_table[i];
        push_to_app_stack(app_esp, init_fn);
      }
      app_init_state.init_fn_table = (init_fn_t**) app_esp;
      break;
    }
  }

  push_to_app_stack(app_esp, app_init_state);

  return app_esp;
}

UserProcess* UserProcess::load(uint8_t* elf_cont) {
  Elf32_Ehdr *ehdr = (Elf32_Ehdr *) elf_cont;
  if (ehdr->e_ident[0] != 0x7F ||
      ehdr->e_ident[1] != 'E' ||
      ehdr->e_ident[2] != 'L' ||
      ehdr->e_ident[3] != 'F') {
    return nullptr;
  }

  // e_entry should be above user_process_va_start
  if (ehdr->e_entry < user_process_va_start) {
    printf("Invalid e_entry 0x%x\n", ehdr->e_entry);
    return nullptr;
  }

  // allocate page dir
  uint32_t pgdir = alloc_phys_page();
  memmove((void*) pgdir, (void*) kernel_page_dir, 4096);
  asm_set_cr3(pgdir);
  Elf32_Phdr* phdrtable = (Elf32_Phdr*) (elf_cont + ehdr->e_phoff);

  for (Elf32_Phdr* phdr = phdrtable; phdr != phdrtable + ehdr->e_phnum; ++phdr) {
    if (phdr->p_type != PT_LOAD) {
      continue;
    }
    // load the segment
    // TODO: map as user R/W so far, but we should map executable segments as read only!
    map_region_alloc(pgdir, phdr->p_vaddr, phdr->p_memsz, MAP_FLAG_USER | MAP_FLAG_WRITE);
    printf("map [%p, %p)\n", phdr->p_vaddr, phdr->p_vaddr + phdr->p_memsz);
   
    // copy file content 
    memmove((void *) phdr->p_vaddr, elf_cont + phdr->p_offset, phdr->p_filesz);

    // clear bss section
    if (phdr->p_filesz < phdr->p_memsz) {
      memset((void*) (phdr->p_vaddr + phdr->p_filesz), 0, phdr->p_memsz - phdr->p_filesz);
    }
  }

  map_region_alloc(pgdir, user_stack_start, 4096 * 7, MAP_FLAG_USER | MAP_FLAG_WRITE);
  UserProcess* proc = UserProcess::allocate();
  proc->pgdir_ = pgdir;
  memset(&proc->intr_frame_, 0, sizeof(proc->intr_frame_));
  proc->intr_frame_.eip = ehdr->e_entry;
  proc->intr_frame_.esp = setup_app_init_state(ehdr);
  proc->intr_frame_.cs = USER_CODE_SEG;
  proc->intr_frame_.ss = USER_DATA_SEG;
  // enable IF
  proc->intr_frame_.eflags = 0x200;

  return proc;
}
