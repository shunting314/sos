#include <stdio.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <vector>
#include "../../../cinc/elf.h"

// TODO may replace these with sos's implementation
#define vector std::vector
#define pair std::pair

/*
 * cur will be updated by this function.
 */
uint32_t read_ULEB128(const uint8_t*& cur, const uint8_t* end) {
  assert(cur < end);
  int shift = 0;
  uint32_t val = 0;
  while (1) {
    uint8_t nval = *cur;
    val = val | ((nval & 0x7F) << shift);
    shift += 7;
    ++cur;

    if ((nval & 0x80) == 0) {
      break;
    }
    assert(cur < end);
    assert(shift + 7 <= 32);
  }
  return val;
}

uint8_t read_uint8(const uint8_t*& cur, const uint8_t* end) {
  assert(cur < end);
  return *cur++;
}

uint16_t read_uint16(const uint8_t*& cur, const uint8_t* end) {
  assert(cur + 1 < end);
  uint16_t ret = *(uint16_t*) cur;
  cur += 2;
  return ret;
}

uint32_t read_uint32(const uint8_t*& cur, const uint8_t* end) {
  assert(cur + 3 < end);
  uint32_t ret = *(uint32_t*) cur;
  cur += 4;
  return ret;
}

const char* read_str(const uint8_t*& cur, const uint8_t* end) {
  assert(cur < end);
  const char* ret = (const char*) cur;
  while (cur < end && *cur) {
    ++cur;
  }
  assert(cur < end && *cur == 0);
  ++cur;
  return ret;
}

void hexdump(const uint8_t *data, int len) {
  for (int i = 0; i < len; ++i) {
    printf(" %02x", data[i]);
    if ((i + 1) % 16 == 0) {
      printf("\n");
    }
  }
  printf("\n");
}

#define FOR_EACH_DW_TAG(_) \
  _(compile_unit, 0x11) \
  _(base_type, 0x24) \
  _(subprogram, 0x2e)

#define FOR_EACH_DW_AT(_) \
  _(name, 0x03) \
  _(byte_size, 0x0b) \
  _(stmt_list, 0x10) \
  _(low_pc, 0x11) \
  _(high_pc, 0x12 /* it actually encodes the number of bytes */) \
  _(language, 0x13) \
  _(comp_dir, 0x1b) \
  _(producer, 0x25) \
  _(prototyped, 0x27) \
  _(decl_column, 0x39) \
  _(decl_file, 0x3a) \
  _(decl_line, 0x3b) \
  _(encoding, 0x3e) \
  _(external, 0x3f) \
  _(frame_base, 0x40) \
  _(type, 0x49) \
  _(call_all_calls, 0x7a) \
  _(call_all_tail_calls, 0x7c)

#define FOR_EACH_DW_FORM(_) \
  _(addr, 0x01) \
  _(data4, 0x06) \
  _(string, 0x08) \
  _(data1, 0x0b) \
  _(strp, 0x0e) \
  _(ref4, 0x13) \
  _(sec_offset, 0x17) \
  _(exprloc, 0x18) \
  _(flag_present, 0x19) \
  _(line_strp, 0x1f)

// standard line number program opcode
#define FOR_EACH_DW_LNS(_) \
  _(advance_pc, 2) \
  _(set_column, 5) \
  _(const_add_pc, 8)

// extended line number program opcode
#define FOR_EACH_DW_LNE(_) \
  _(end_sequence, 1) \
  _(set_address, 2)

enum EnumDW_TAG {
#define ENUM(name, val) DW_TAG_ ## name = val,
FOR_EACH_DW_TAG(ENUM)
#undef ENUM
};

const char* dwtag2str(uint32_t tag) {
  switch (tag) {
  #define CASE(name, _) case DW_TAG_ ## name: return "DW_TAG_" # name;
    FOR_EACH_DW_TAG(CASE)
  #undef CASE
  default:
    printf("Unrecognized tag 0x%x\n", tag);
    assert(false);
    return NULL;
  }
}

enum EnumDW_AT {
#define ENUM(name, val) DW_AT_ ## name = val,
FOR_EACH_DW_AT(ENUM)
#undef ENUM
};

const char* dwat2str(uint32_t attr) {
  switch (attr) {
  #define CASE(name, _) case DW_AT_ ## name: return "DW_AT_" # name;
    FOR_EACH_DW_AT(CASE)
  #undef CASE
  default:
    printf("Unrecognized attr 0x%x\n", attr);
    assert(false);
    return NULL;
  }
}

enum EnumDW_FORM {
#define ENUM(name, val) DW_FORM_ ## name = val,
FOR_EACH_DW_FORM(ENUM)
#undef ENUM
};

const char* dwform2str(uint32_t form) {
  switch (form) {
  #define CASE(name, _) case DW_FORM_ ## name: return "DW_FORM_" # name;
    FOR_EACH_DW_FORM(CASE)
  #undef CASE
  default:
    printf("Unrecognized form 0x%x\n", form);
    assert(false);
    return NULL;
  }
}

enum EnumDW_LNS {
#define ENUM(name, val) DW_LNS_ ## name = val,
FOR_EACH_DW_LNS(ENUM)
#undef ENUM
};

const char* dwlns2str(uint32_t opcode) {
  switch (opcode) {
  #define CASE(name, _) case DW_LNS_ ## name: return "DW_LNS_" # name;
    FOR_EACH_DW_LNS(CASE)
  #undef CASE
  default:
    printf("Unrecognized standard opcode 0x%x\n", opcode);
    assert(false);
    return NULL;
  }
}

enum EnumDW_LNE {
#define ENUM(name, val) DW_LNE_ ## name = val,
FOR_EACH_DW_LNE(ENUM)
#undef ENUM
};

const char* dwlne2str(uint32_t opcode) {
  switch (opcode) {
  #define CASE(name, _) case DW_LNE_ ## name: return "DW_LNE_" # name;
    FOR_EACH_DW_LNE(CASE)
  #undef CASE
  default:
    printf("Unrecognized extended opcode 0x%x\n", opcode);
    assert(false);
    return NULL;
  }
}

#define DW_CHILDREN_no 0x00
#define DW_CHILDREN_yes 0x01

#define DW_UT_compile 0x01

class AbbrevEntry {
 public:
  explicit AbbrevEntry(uint32_t code, uint32_t tag, bool has_child)
    : code_(code),
      tag_(tag),
      has_child_(has_child) { }

  void add_pair(uint32_t attr, uint32_t form) {
    attr_form_pairs_.emplace_back(attr, form);
  }

  uint32_t tag() const { return tag_; };

  void dump() const {
    printf("code %d, tag %s, has_child %d, #pairs %lu\n", code_, dwtag2str(tag_), has_child_, attr_form_pairs_.size());
    for (auto& kv : attr_form_pairs_) {
      printf("  attr %s, form %s\n", dwat2str(kv.first), dwform2str(kv.second));
    }
  }

  const vector<pair<uint32_t, uint32_t>>& get_attr_form_pairs() const {
    return attr_form_pairs_;
  }
 private:
  uint32_t code_;
  uint32_t tag_;
  /*
   * The next DIE is the first child if has_child_ is true, otherwise it's the
   * next sibling. The sibling list ends with an entry with 0 code.
   *
   * The DIE list is a pre-order traversal of the DIE tree. Leveraging this flag,
   * we can reconstruct the tree.
   */
  bool has_child_;
  vector<pair<uint32_t, uint32_t>> attr_form_pairs_;

  friend class AbbrevTable;
};

class AbbrevTable {
 public:
  explicit AbbrevTable() { }

  void parse(const uint8_t* debug_abbrev_buf, int debug_abbrev_nbytes) {
    printf(".debug_abbrev content:\n");
    hexdump(debug_abbrev_buf, debug_abbrev_nbytes);
  
    const uint8_t* cur = debug_abbrev_buf, *end = debug_abbrev_buf + debug_abbrev_nbytes;
    while (cur < end) {
      uint32_t abbrev_code = read_ULEB128(cur, end);
      if (abbrev_code == 0) {
        break;
      }
      // make sure no duplicate
      assert(find(abbrev_code) == NULL);
      uint32_t tag = read_ULEB128(cur, end);
      bool has_child = read_uint8(cur, end);

      entries_.emplace_back(abbrev_code, tag, has_child);

      AbbrevEntry& entry = entries_.back();
  
      while (1) {
        uint32_t attr = read_ULEB128(cur, end);
        uint32_t form = read_ULEB128(cur, end);
        if (attr == 0) {
          assert(form == 0);
          break;
        }
        entry.add_pair(attr, form);
      }
    }
    assert(cur == end);
  }

  void dump() const {
    printf("Got %lu abbrev entries:\n", entries_.size());
    for (auto& entry : entries_) {
      entry.dump();
    }
  }

  /*
   * XXX use hash table.
   */
  const AbbrevEntry* find(uint32_t code) const {
    for (const auto& entry : entries_) {
      if (entry.code_ == code) {
        return &entry;
      }
    }
    return NULL;
  }
 private:
  vector<AbbrevEntry> entries_;
};

struct FunctionEntry {
  const char* name;
  uint32_t start;
  uint32_t len;
};

class DwarfContext {
 public:
  void parse_abbrev_table(const uint8_t* debug_abbrev_buf, int debug_abbrev_nbytes) {
    abbrev_table_.parse(debug_abbrev_buf, debug_abbrev_nbytes);
    abbrev_table_.dump();
  }
  void parse_debug_info(const uint8_t* debug_info_buf, int debug_info_nbytes);
  void parse_debug_line(const uint8_t* debug_line_buf, int debug_line_nbytes);
  void parse_elf(uint8_t* elfbuf, int filesiz);
 private:
  void add_lntab_entry(uint32_t addr, uint32_t ln) {
    printf("addr 0x%x -> line number %d\n", addr, ln); // TODO
    lntab_.emplace_back(addr, ln);
  }

  AbbrevTable abbrev_table_;
  uint8_t* debug_str_buf_;
  uint32_t debug_str_nbytes_;
  uint8_t* debug_line_str_buf_;
  uint32_t debug_line_str_nbytes_;

  vector<FunctionEntry> function_entries_;
  vector<pair<uint32_t, uint32_t>> lntab_;
};

/* 
 * 32 bit dwarf format's unit headers start with the following common fields:
 * - 4 byte length field for the payload followed. 64 bit dwarf format uses 12 bytes.
 * - 2 byte version number.
 * - 1 byte for unit type.
 *
 * For a compilation unit header, the following fields follow:
 * - 1 byte address size
 * - 4 byte debug abbrev offset (8 byte for 64 bit dwarf)
 */
void DwarfContext::parse_debug_info(const uint8_t* debug_info_buf, int debug_info_nbytes) {
  printf(".debug_info content:\n");
  hexdump(debug_info_buf, debug_info_nbytes);

  uint32_t len = *(uint32_t*) debug_info_buf;  // little endian
  assert(len + 4 == debug_info_nbytes);

  const uint8_t* cur = debug_info_buf + 4, *end = debug_info_buf + debug_info_nbytes;
  uint16_t version = read_uint16(cur, end);
  assert(version == 5 && "assume it's DWARF v5");
  uint8_t unit_type = read_uint8(cur, end);
  assert(unit_type == DW_UT_compile && "assum DW_UT_compile unit type");
  uint8_t address_size = read_uint8(cur, end);
  assert(address_size == 4);
  uint32_t debug_abbrev_offset = read_uint32(cur, end);
  assert(debug_abbrev_offset == 0);

  while (cur < end) {
    uint32_t abbrev_code = read_ULEB128(cur, end);
    printf("Got abbrev_code %d\n", abbrev_code);
    if (!abbrev_code) {
      continue;
    }
    const AbbrevEntry* abbrev_entry = abbrev_table_.find(abbrev_code);
    assert(abbrev_entry);
    abbrev_entry->dump();

    const char* fn_name = NULL;
    uint32_t lowpc = -1, fn_nbytes = -1;

    for (const auto& attr_form_pair : abbrev_entry->get_attr_form_pairs()) {
      uint32_t attr = attr_form_pair.first;
      uint32_t form = attr_form_pair.second;
      switch (form) {
      case DW_FORM_strp: {
        uint32_t off = read_uint32(cur, end);
        const char* str = (const char*) debug_str_buf_ + off;
        printf("strp: '%s'\n", str);

        if (attr == DW_AT_name) {
          fn_name = str;
        }
        break;
      }
      case DW_FORM_line_strp: {
        uint32_t off = read_uint32(cur, end);
        printf("line_strp: '%s'\n", debug_line_str_buf_ + off);
        break;
      }
      case DW_FORM_data1: {
        uint8_t data = read_uint8(cur, end);
        printf("data1: %u\n", data);
        break;
      }
      case DW_FORM_data4: {
        uint32_t data = read_uint32(cur, end);
        printf("data4: %d (0x%x) \n", data, data);

        if (attr == DW_AT_high_pc) {
          fn_nbytes = data;
        }
        break;
      }
      case DW_FORM_addr: {
        uint32_t addr = read_uint32(cur, end);
        printf("addr: 0x%x\n", addr);

        if (attr == DW_AT_low_pc) {
          lowpc = addr;
        }
        break;
      }
      case DW_FORM_exprloc: {
        uint32_t len = read_ULEB128(cur, end);
        printf("exprloc: %d bytes\n", len);
        cur += len;
        assert(cur <= end);
        break;
      }
      case DW_FORM_flag_present: {
        // no data need to be encoded
        printf("flag_present\n");
        break;
      }
      case DW_FORM_ref4: {
        uint32_t ref = read_uint32(cur, end);
        printf("ref4: 0x%x\n", ref);
        break;
      }
      case DW_FORM_sec_offset: {
        uint32_t off = read_uint32(cur, end);
        printf("sec_offset: 0x%x\n", off);
        break;
      }
      case DW_FORM_string: {
        const char* str = read_str(cur, end);
        printf("string: '%s'\n", str);

        if (attr == DW_AT_name) {
          fn_name = str;
        }
        break;
      }
      default:
        printf("Unrecognized form %s\n", dwform2str(form));
        assert(false && "unrecognized form");
      }
    }
  
    // add the function entry
    if (abbrev_entry->tag() == DW_TAG_subprogram) {
      assert(fn_name);
      assert(lowpc != -1);
      assert(fn_nbytes != -1);
      function_entries_.push_back({fn_name, lowpc, fn_nbytes});
    }
  }
  assert(cur == end);

  // dump the function entries
  printf("Found %lu functions\n", function_entries_.size());
  for (auto& entry : function_entries_) {
    printf("- '%s', 0x%x, %d\n", entry.name, entry.start, entry.len);
  }
}

/*
 * Refer to DWARF5 spec section 6.2.4. for how to parse the header.
 */
void DwarfContext::parse_debug_line(const uint8_t* debug_line_buf, int debug_line_nbytes) {
  printf(".debug_line content:\n");
  hexdump(debug_line_buf, debug_line_nbytes);
  uint32_t len = *(uint32_t*) debug_line_buf;
  assert(len + 4 == debug_line_nbytes);

  const uint8_t* cur = debug_line_buf + 4, *end = debug_line_buf + debug_line_nbytes;
  // this is the version of the line number program.
  // It is independent to the DWARF spec's version
  uint16_t version = read_uint16(cur, end);
  uint8_t address_size = read_uint8(cur, end);
  assert(address_size == 4);
  uint8_t segment_selector_size = read_uint8(cur, end);
  // TODO use this to do an assertion
  uint32_t header_length = read_uint32(cur, end);
  // the size in bytes of the smallest target machine instruction.
  uint8_t minimum_instruction_length = read_uint8(cur, end);
  assert(minimum_instruction_length == 1);
  // used to support VLIW
  uint8_t maximum_operations_per_instruction = read_uint8(cur, end);
  assert(maximum_operations_per_instruction == 1);
  uint8_t default_is_stmt = read_uint8(cur, end);
  int8_t line_base = (int8_t) read_uint8(cur, end);
  uint8_t line_range = read_uint8(cur, end);
  // the number assigned to the first special opcode.
  uint8_t opcode_base = read_uint8(cur, end);

  // skip standard opcode lengths
  for (int i = 1; i <= opcode_base - 1; ++i) {
    read_uint8(cur, end);
  }

  uint8_t directory_entry_format_count = read_uint8(cur, end);
  for (int i = 0; i < directory_entry_format_count; ++i) {
    read_ULEB128(cur, end);
    read_ULEB128(cur, end);
  }
  uint32_t directories_count = read_ULEB128(cur, end);
  printf("directories_count %d\n", directories_count);
  for (int i = 0; i < directories_count; ++i) {
    read_uint32(cur, end);
  }
  uint8_t file_name_entry_format_count = read_uint8(cur, end);
  for (int i = 0; i < file_name_entry_format_count; ++i) {
    read_ULEB128(cur, end);
    read_ULEB128(cur, end);
  }

  uint32_t file_names_count = read_ULEB128(cur, end);
  for (int i = 0; i < file_names_count; ++i) {
    read_uint32(cur, end); // name
    read_ULEB128(cur, end); // directory. Should be uint8?
  }

  // For simplicities, I assume each instruction only contains one operation.
  // This is true for x86 (and most likely also be true for arm. but I need confirm)
  uint8_t cur_line = 1, cur_addr = 0;
  add_lntab_entry(cur_addr, cur_line);
  while (cur < end) {
    uint8_t byte0 = read_uint8(cur, end);
    if (byte0 == 0) {
      uint32_t extlen = read_ULEB128(cur, end);
      uint32_t opcode = read_ULEB128(cur, end);
      switch (opcode) {
      case DW_LNE_end_sequence:
        break;
      case DW_LNE_set_address:
        cur_addr = read_uint32(cur, end);
        add_lntab_entry(cur_addr, cur_line);
        break;
      default:
        printf("Got unsupported extended opcode %d (%s)\n", opcode, dwlne2str(opcode));
        assert(false);
      }
    } else if (byte0 < opcode_base) {
      switch (byte0) {
      case DW_LNS_advance_pc:
        cur_addr += read_ULEB128(cur, end);
        add_lntab_entry(cur_addr, cur_line);
        break;
      case DW_LNS_set_column:
        read_ULEB128(cur, end); // ignore column for now
        break;
      case DW_LNS_const_add_pc:
        // add pc according to a const opcode of 255
        cur_addr += (255 - opcode_base) / line_range;
        add_lntab_entry(cur_addr, cur_line);
        break;
      default:
        printf("Got unsupported standard opcode %d (%s)\n", byte0, dwlns2str(byte0));
        assert(false);
      }
    } else {
      assert(byte0 >= opcode_base);
      byte0 -= opcode_base;
      cur_line += byte0 % line_range + line_base;
      cur_addr += byte0 / line_range;
      add_lntab_entry(cur_addr, cur_line);
    }
  }

  assert(cur == end);
}

void DwarfContext::parse_elf(uint8_t* elfbuf, int filesiz) {
  Elf32_Ehdr* elfhdr = (Elf32_Ehdr*) elfbuf;
  Elf32_Shdr* shdrtab = (Elf32_Shdr*) (elfbuf + elfhdr->e_shoff);
  char* shstrtab = (char*) (elfbuf + shdrtab[elfhdr->e_shstrndx].sh_offset);
  int nsec = elfhdr->e_shnum;

  Elf32_Shdr* debug_abbrev_shdr = NULL;
  Elf32_Shdr* debug_info_shdr = NULL;
  Elf32_Shdr* debug_str_shdr = NULL;
  Elf32_Shdr* debug_line_shdr = NULL;
  Elf32_Shdr* debug_line_str_shdr = NULL;

  printf("#%d sections\n", nsec);
  for (int i = 0; i < nsec; ++i) {
    Elf32_Shdr* cursec = shdrtab + i;
    const char* secname = shstrtab + cursec->sh_name;
    printf(" - %s\n", secname);
    if (strcmp(secname, ".debug_info") == 0) {
      debug_info_shdr = cursec;
    } else if (strcmp(secname, ".debug_abbrev") == 0) {
      debug_abbrev_shdr = cursec;
    } else if (strcmp(secname, ".debug_str") == 0) {
      debug_str_shdr = cursec;
    } else if (strcmp(secname, ".debug_line") == 0) {
      debug_line_shdr = cursec;
    } else if (strcmp(secname, ".debug_line_str") == 0) {
      debug_line_str_shdr = cursec;
    }
  }
  assert(debug_abbrev_shdr != NULL);
  assert(debug_info_shdr != NULL);
  assert(debug_str_shdr != NULL);

  uint8_t* debug_abbrev_buf = elfbuf + debug_abbrev_shdr->sh_offset;
  int debug_abbrev_nbytes = debug_abbrev_shdr->sh_size;
  parse_abbrev_table(debug_abbrev_buf, debug_abbrev_nbytes);

  debug_str_buf_ = elfbuf + debug_str_shdr->sh_offset;
  debug_str_nbytes_ = debug_str_shdr->sh_size;
  printf(".debug_str content:\n");
  hexdump(debug_str_buf_, debug_str_nbytes_);

  debug_line_str_buf_ = elfbuf + debug_line_str_shdr->sh_offset;
  debug_line_str_nbytes_ = debug_line_str_shdr->sh_size;

  uint8_t* debug_info_buf = elfbuf + debug_info_shdr->sh_offset;
  int debug_info_nbytes = debug_info_shdr->sh_size;
  parse_debug_info(debug_info_buf, debug_info_nbytes);

  uint8_t* debug_line_buf = elfbuf + debug_line_shdr->sh_offset;
  int debug_line_nbytes = debug_line_shdr->sh_size;
  parse_debug_line(debug_line_buf, debug_line_nbytes);
}

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

  DwarfContext ctx;
  ctx.parse_elf(buf, filesiz);

  free(buf);
  printf("Bye\n");
  return 0;
}
