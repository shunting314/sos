/*
 * There is similar logic to load kernel ELF content from the disk in kernel/init.cpp .
 * Don't plan to unify them. The code in kernel/init.cpp uses classes like IDEDevice to make the code easier to read. But we don't want to use that here to avoid increasing binary size for the bootloader.
 */

#include <stdint.h>
#include <elf.h>

#define SECTOR_SIZE 512
// the first kernel sector is sector 1 (number starts from 0)
#define FIRST_KERNEL_SECTOR 1
#define IO_PORT_BASE 0x1F0
#define DATA_PORT IO_PORT_BASE
#define ERROR_REG_PORT (IO_PORT_BASE + 1)
#define SECT_COUNT_PORT (IO_PORT_BASE + 2)
#define LBA_LOW_PORT (IO_PORT_BASE + 3)
#define LBA_MID_PORT (IO_PORT_BASE + 4)
#define LBA_HIGH_PORT (IO_PORT_BASE + 5)
#define DEVICE_REG_PORT (IO_PORT_BASE + 6)
#define STATUS_COMMAND_PORT (IO_PORT_BASE + 7)

#define CMD_READ_SECTORS 0x20

#define BSY 7 // busy

static inline void fatal() {
  while (1) {
  }
}

static inline void outb(int port, uint8_t byte) {
  asm volatile("outb %1, %0" : : "d"(port), "a"(byte));
}

static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  asm volatile(
      "inb %1, %0" : "=a"(ret) : "d"(port) /* port should go to edx */:
  );
  return ret;
}

static inline uint16_t inw(int port) {
  uint16_t ret;
  // must use volatile to avoid compiler moving inw out of a loop
  asm volatile(
      "inw %1, %0" : "=a"(ret) : "d"(port) /* port should go to edx */:
  );
  return ret;
}

static inline void waitUntilNotBusy() {
  while (true) {
    uint8_t status = inb(STATUS_COMMAND_PORT);
    if (!((status >> BSY) & 0x1)) {
      break;
    }
  }
}

static inline void read_sector(char *dst_addr, int sector_no) {
  outb(SECT_COUNT_PORT, 1);

  // set lba
  outb(LBA_LOW_PORT, sector_no & 0xFF);
  outb(LBA_MID_PORT, (sector_no >> 8) & 0xFF);
  outb(LBA_HIGH_PORT, (sector_no >> 16) & 0xFF);
  outb(DEVICE_REG_PORT, (0xE0 | ((sector_no >> 24) & 0xF))); // master
  outb(STATUS_COMMAND_PORT, CMD_READ_SECTORS);

  // it's crucial to wait for data to be ready before reading *EACH* sector
  waitUntilNotBusy();
  for (int i = 0; i < SECTOR_SIZE; i += 2) {
    *((uint16_t*) &dst_addr[i]) = inw(DATA_PORT);
  }
}

void load_from_disk(char *dst_addr, int file_off, int size) {
  // only handle simple case that file_off sitting on sector boundary
  if (file_off % SECTOR_SIZE != 0) {
    fatal();
  }
  // NOTE: we may write more to dst_addr if size is not a multiple of SECTOR_SIZE
  for (int rel_off = 0; rel_off < size; rel_off += SECTOR_SIZE, dst_addr += SECTOR_SIZE) {
    int sector_no = (file_off + rel_off) / SECTOR_SIZE + FIRST_KERNEL_SECTOR;
    read_sector(dst_addr, sector_no);
  }
}

void load_kernel_and_enter() {
  char buf[4096];
  load_from_disk(buf, 0, sizeof(buf));
  Elf32_Ehdr *elf_hdr = (Elf32_Ehdr *) buf;

  if (elf_hdr->e_ident[0] != 0x7f || 
      elf_hdr->e_ident[1] != 'E' ||
      elf_hdr->e_ident[2] != 'L' ||
      elf_hdr->e_ident[3] != 'F') {
    fatal();
  }

  // make sure the program header table is read in the buffer already
  if (elf_hdr->e_phoff + elf_hdr->e_phnum * sizeof(Elf32_Phdr) > sizeof(buf)) {
    fatal();
  }

  Elf32_Phdr* phdrtable = (void *) buf + elf_hdr->e_phoff;
  for (Elf32_Phdr* phdr = phdrtable; phdr != phdrtable + elf_hdr->e_phnum; ++phdr) {
    // loadable segment
    if (phdr->p_type == PT_LOAD) {
      // memcpy
      char *dst_addr = (void *) phdr->p_vaddr;

      load_from_disk(dst_addr, phdr->p_offset, phdr->p_filesz);
      dst_addr += phdr->p_filesz;

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
