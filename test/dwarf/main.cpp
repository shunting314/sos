#include <stdio.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../../cinc/dwarf.h"

int main(int argc, char** argv) {
  if (argc != 2) {
    printf("Usage: %s path_to_elf_file\n", argv[0]);
    return -1;
  }
  printf("Path %s\n", argv[1]);
  struct stat file_stat;
  int status = stat(argv[1], &file_stat);
  assert(status == 0 && "File not found");
  int filesiz = file_stat.st_size;

  printf("File size %d\n", filesiz);
  FILE* fp = fopen(argv[1], "rb");
  assert(fp);

  uint8_t *buf;
  buf = (uint8_t*) malloc(filesiz);
  assert(buf);

  status = fread(buf, 1, filesiz, fp);
  assert(status == filesiz);

  if (filesiz < 4 || buf[0] != 0x7F || buf[1] != 'E' || buf[2] != 'L' || buf[3] != 'F') {
    printf("Invalid elf file\n");
    return -1;
  }

  DwarfContext ctx(buf);

  #if 1
  // testing for the addresses printed in sos kernel backtrace
  // these may become invalid as sos kernel keeps evolving.
  uint32_t addrlist[] = {
    0x10942e,
    0x10a2e0,
    0,
  };
  for (int i = 0; addrlist[i]; ++i) {
    uint32_t addr = addrlist[i];
    const FunctionEntry* entry = ctx.find_func_entry_by_addr(addr);
    const LineNoEntry* lnentry = ctx.find_lineno_entry_by_addr(addr);
    assert(lnentry);
    printf("addr 0x%x -> %s, %s:%d\n", addr, entry ? entry->name : "<unknown>", lnentry->file_name, lnentry->lineno);
  }
  #endif

  free(buf);
  printf("Bye\n");
  return 0;
}
