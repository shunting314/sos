#pragma once

#include <assert.h>

struct dirent {
  char name[64];
  uint32_t file_size;
  int8_t ent_type;
};

enum DIR_ENT_TYPE {
  ET_FILE, // regular file
  ET_DIR, // subdirectory
  ET_NOEXIST,
};

static const char* dirent_typestr(int ent_type) {
  switch (ent_type) {
    case ET_FILE:
      return "f";
    case ET_DIR:
      return "d";
    default:
      assert(false && "Invalid DirEnt type");
      return nullptr;
  }
}
