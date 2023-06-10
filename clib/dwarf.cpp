#ifdef HOST_OS
#include "../cinc/dwarf.h"
#include "../cinc/elf.h"
#else
#include <dwarf.h>
#include <elf.h>
#endif

#include <string.h>

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

int32_t read_SLEB128(const uint8_t*& cur, const uint8_t* end) {
  const uint8_t* oldcur = cur;
  uint32_t uval = read_ULEB128(cur, end);
  int nbyte = cur - oldcur;
  int nbit = nbyte * 7;
  assert(nbit <= 31);
  if (uval & (1 << (nbit - 1))) {
    uval |= -(1 << nbit);
  }
  return (int32_t) uval;
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

const char* dwtag2str(uint32_t tag) {
  switch (tag) {
  #define CASE(name, _) case DW_TAG_ ## name: return "DW_TAG_" # name;
    FOR_EACH_DW_TAG(CASE)
  #undef CASE
  default:
    printf("Unrecognized tag 0x%x\n", tag);
    assert(false);
    return nullptr;
  }
}

const char* dwat2str(uint32_t attr) {
  switch (attr) {
  #define CASE(name, _) case DW_AT_ ## name: return "DW_AT_" # name;
    FOR_EACH_DW_AT(CASE)
  #undef CASE
  default:
    printf("Unrecognized attr 0x%x\n", attr);
    assert(false);
    return nullptr;
  }
}

const char* dwform2str(uint32_t form) {
  switch (form) {
  #define CASE(name, _) case DW_FORM_ ## name: return "DW_FORM_" # name;
    FOR_EACH_DW_FORM(CASE)
  #undef CASE
  default:
    printf("Unrecognized form 0x%x\n", form);
    assert(false);
    return nullptr;
  }
}

const char* dwlns2str(uint32_t opcode) {
  switch (opcode) {
  #define CASE(name, _) case DW_LNS_ ## name: return "DW_LNS_" # name;
    FOR_EACH_DW_LNS(CASE)
  #undef CASE
  default:
    printf("Unrecognized standard opcode 0x%x\n", opcode);
    assert(false);
    return nullptr;
  }
}

const char* dwlne2str(uint32_t opcode) {
  switch (opcode) {
  #define CASE(name, _) case DW_LNE_ ## name: return "DW_LNE_" # name;
    FOR_EACH_DW_LNE(CASE)
  #undef CASE
  default:
    printf("Unrecognized extended opcode 0x%x\n", opcode);
    assert(false);
    return nullptr;
  }
}

void DwarfContext::parse_elf(uint8_t* elfbuf) {
  if (elfbuf[0] != 0x7F || elfbuf[1] != 'E' || elfbuf[2] != 'L' || elfbuf[3] != 'F') {
    safe_assert(false && "Invalid elf file\n");
  }

  Elf32_Ehdr* elfhdr = (Elf32_Ehdr*) elfbuf;
  Elf32_Shdr* shdrtab = (Elf32_Shdr*) (elfbuf + elfhdr->e_shoff);
  char* shstrtab = (char*) (elfbuf + shdrtab[elfhdr->e_shstrndx].sh_offset);
  int nsec = elfhdr->e_shnum;

  Elf32_Shdr* debug_abbrev_shdr = nullptr;
  Elf32_Shdr* debug_info_shdr = nullptr;
  Elf32_Shdr* debug_str_shdr = nullptr;
  Elf32_Shdr* debug_line_shdr = nullptr;
  Elf32_Shdr* debug_line_str_shdr = nullptr;

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
  safe_assert(debug_abbrev_shdr != nullptr);
  safe_assert(debug_info_shdr != nullptr);
  safe_assert(debug_str_shdr != nullptr);
  safe_assert(debug_line_shdr != nullptr);
  safe_assert(debug_line_str_shdr != nullptr);

  uint8_t* debug_abbrev_buf = elfbuf + debug_abbrev_shdr->sh_offset;
  int debug_abbrev_nbytes = debug_abbrev_shdr->sh_size;
  parse_abbrev_table(debug_abbrev_buf, debug_abbrev_nbytes);

  debug_str_buf_ = elfbuf + debug_str_shdr->sh_offset;
  debug_str_nbytes_ = debug_str_shdr->sh_size;
  if (DEBUG) {
    printf(".debug_str content:\n");
    hexdump(debug_str_buf_, debug_str_nbytes_);
  }

  debug_line_str_buf_ = elfbuf + debug_line_str_shdr->sh_offset;
  debug_line_str_nbytes_ = debug_line_str_shdr->sh_size;

  uint8_t* debug_info_buf = elfbuf + debug_info_shdr->sh_offset;
  int debug_info_nbytes = debug_info_shdr->sh_size;
  parse_debug_info(debug_info_buf, debug_info_nbytes);

  uint8_t* debug_line_buf = elfbuf + debug_line_shdr->sh_offset;
  int debug_line_nbytes = debug_line_shdr->sh_size;
  parse_debug_line(debug_line_buf, debug_line_nbytes);
}

/*
 * cur points after length field when this method is called.
 *
 * 'start' points to the start of the .debug_info section. Pass this in so we can
 * print the relative offset from 'start' for debugging.
 */
void DwarfContext::parse_debug_info_for_one_abbrev_off(const uint8_t* start, const uint8_t* cur, const uint8_t* end) {
  uint16_t version = read_uint16(cur, end);
  assert(version == 5 && "assume it's DWARF v5");
  uint8_t unit_type = read_uint8(cur, end);
  assert(unit_type == DW_UT_compile && "assum DW_UT_compile unit type");
  uint8_t address_size = read_uint8(cur, end);
  assert(address_size == 4);
  uint32_t debug_abbrev_offset = read_uint32(cur, end);

  const vector<AbbrevEntry>* pentries = abbrev_table_.find_by_off(debug_abbrev_offset);
  assert(pentries);

  while (cur < end) {
    dprintf("Handle .debug_info offset 0x%lx\n", (cur - start));
    uint32_t abbrev_code = read_ULEB128(cur, end);
    dprintf("Got abbrev_code %d\n", abbrev_code);
    if (!abbrev_code) {
      continue;
    }
    const AbbrevEntry* abbrev_entry = abbrev_table_.find(*pentries, abbrev_code);
    assert(abbrev_entry);
    if (DEBUG) {
      abbrev_entry->dump();
    }

    const char* fn_name = nullptr;
    uint32_t lowpc = -1, fn_nbytes = -1;

    for (const auto& attr_form_pair : abbrev_entry->get_attr_form_pairs()) {
      uint32_t attr = attr_form_pair.first;
      uint32_t form = attr_form_pair.second;

      // TODO decouple the association of an attribute with a specific form
      switch (form) {
      case DW_FORM_strp: {
        uint32_t off = read_uint32(cur, end);
        const char* str = (const char*) debug_str_buf_ + off;
        dprintf("strp: '%s'\n", str);

        if (attr == DW_AT_name) {
          fn_name = str;
        }
        break;
      }
      case DW_FORM_line_strp: {
        uint32_t off = read_uint32(cur, end);
        dprintf("line_strp: '%s'\n", debug_line_str_buf_ + off);
        break;
      }
      case DW_FORM_data1: {
        uint8_t data = read_uint8(cur, end);
        dprintf("data1: %u\n", data);
        break;
      }
      case DW_FORM_data2: {
        uint16_t data = read_uint16(cur, end);
        dprintf("data2: %u\n", data);
        break;
      }
      case DW_FORM_data4: {
        uint32_t data = read_uint32(cur, end);
        dprintf("data4: %d (0x%x) \n", data, data);

        if (attr == DW_AT_high_pc) {
          fn_nbytes = data;
        }
        break;
      }
      case DW_FORM_addr: {
        uint32_t addr = read_uint32(cur, end);
        dprintf("addr: 0x%x\n", addr);

        if (attr == DW_AT_low_pc) {
          lowpc = addr;
        }
        break;
      }
      case DW_FORM_exprloc: {
        uint32_t len = read_ULEB128(cur, end);
        dprintf("exprloc: %d bytes\n", len);
        cur += len;
        assert(cur <= end);
        break;
      }
      case DW_FORM_flag_present: {
        // no data need to be encoded
        dprintf("flag_present\n");
        break;
      }
      case DW_FORM_implicit_const: {
        dprintf("implicit_const: TODO store the implicit const data with the abbrev entry\n");
        break;
      }
      case DW_FORM_ref4: {
        uint32_t ref = read_uint32(cur, end);
        dprintf("ref4: 0x%x\n", ref);
        break;
      }
      case DW_FORM_sec_offset: {
        uint32_t off = read_uint32(cur, end);
        dprintf("sec_offset: 0x%x\n", off);
        break;
      }
      case DW_FORM_string: {
        const char* str = read_str(cur, end);
        dprintf("string: '%s'\n", str);

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
    if (abbrev_entry->tag() == DW_TAG_subprogram && fn_name && (lowpc != -1 || fn_nbytes != -1)) {
      assert(fn_name);
      assert(lowpc != -1);
      assert(fn_nbytes != -1);
      function_entries_.push_back({fn_name, lowpc, fn_nbytes});
    }
  }
  assert(cur == end);
}

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
  if (DEBUG) {
    hexdump(debug_info_buf, debug_info_nbytes);
  }

  const uint8_t* cur = debug_info_buf, *end = debug_info_buf + debug_info_nbytes;
  while (cur < end) {
    uint32_t len = read_uint32(cur, end);
    assert(cur + len <= end);

    parse_debug_info_for_one_abbrev_off(debug_info_buf, cur, cur + len);
    cur = cur + len;
  }
  assert(cur == end);

  // dump the function entries
  printf("Found %lu functions\n", function_entries_.size());
  for (auto& entry : function_entries_) {
    printf("- '%s', 0x%x, %d\n", entry.name, entry.start, entry.len);
  }
}

/*
 * cur points after length field when this method is called.
  */
void DwarfContext::parse_debug_line_one_program(const uint8_t* start, const uint8_t* cur, const uint8_t* end) {
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
    uint32_t off = read_uint32(cur, end);
    printf("  dir %s\n", debug_line_str_buf_ + off);
  }
  uint8_t file_name_entry_format_count = read_uint8(cur, end);
  printf("file name entry format count %d\n", file_name_entry_format_count);
  for (int i = 0; i < file_name_entry_format_count; ++i) {
    read_ULEB128(cur, end);
    read_ULEB128(cur, end);
  }

  uint32_t file_names_count = read_ULEB128(cur, end);
  vector<const char*> file_names;
  printf("file name count %d\n", file_names_count);
  for (int i = 0; i < file_names_count; ++i) {
    uint32_t off = read_uint32(cur, end); // name
    read_ULEB128(cur, end); // directory. Should be uint8?
    printf("  filename %s\n", debug_line_str_buf_ + off);
    file_names.push_back((const char*) debug_line_str_buf_ + off);
  }

  printf("current off 0x%lx\n", cur - start); // TODO
  // For simplicities, I assume each instruction only contains one operation.
  // This is true for x86 (and most likely also be true for arm. but I need confirm)
  uint32_t cur_line = 1, cur_addr = 0;
  uint32_t cur_file_idx = 0;
  const char* cur_file_name = file_names[0];
  uint32_t discriminator;
  // add_lntab_entry(cur_addr, cur_line); // don't add the initial state
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
        add_lntab_entry(cur_addr, cur_file_name, cur_line);
        break;
      case DW_LNE_set_discriminator:
        discriminator = read_ULEB128(cur, end); // ignore for now
        dprintf("Ignore discriminator %d\n", discriminator);
        break;
      default:
        printf("Got unsupported extended opcode %d (%s)\n", opcode, dwlne2str(opcode));
        assert(false);
      }
    } else if (byte0 < opcode_base) {
      switch (byte0) {
      case DW_LNS_copy:
        // nop for now
        break;
      case DW_LNS_advance_pc:
        cur_addr += read_ULEB128(cur, end);
        add_lntab_entry(cur_addr, cur_file_name, cur_line);
        break;
      case DW_LNS_advance_line:
        cur_line += read_SLEB128(cur, end);
        add_lntab_entry(cur_addr, cur_file_name, cur_line);
        break;
      case DW_LNS_set_file:
        cur_file_idx = read_ULEB128(cur, end);
        printf("Set file idx to %d\n", cur_file_idx);
        cur_file_name = file_names[cur_file_idx];
        break;
      case DW_LNS_negate_stmt:
        break; // ignore
      case DW_LNS_set_column:
        read_ULEB128(cur, end); // ignore column for now
        break;
      case DW_LNS_const_add_pc:
        // add pc according to a const opcode of 255
        cur_addr += (255 - opcode_base) / line_range;
        add_lntab_entry(cur_addr, cur_file_name, cur_line);
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
      add_lntab_entry(cur_addr, cur_file_name, cur_line);
    }
  }
  assert(cur == end);
}

/*
 * Refer to DWARF5 spec section 6.2.4. for how to parse the header.
 */
void DwarfContext::parse_debug_line(const uint8_t* debug_line_buf, int debug_line_nbytes) {
  printf("parsing .debug_line:\n");
  // hexdump(debug_line_buf, debug_line_nbytes);

  const uint8_t* cur = debug_line_buf;
  const uint8_t* end = debug_line_buf + debug_line_nbytes;

  while (cur < end) {
    uint32_t len = read_uint32(cur, end);
    assert(cur + len <= end);

    parse_debug_line_one_program(debug_line_buf, cur, cur + len);
    cur = cur + len;
  }
  assert(cur == end);

  postprocess_lntab();
  dump_lntab();
}
