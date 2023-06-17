#pragma once

#include <stdint.h>
#include <stdlib.h>

#ifdef HOST_OS
#include <vector>
using std::pair;
using std::vector;
using std::make_pair;
#define safe_assert assert
static void hexdump(const uint8_t *data, int len) {
  for (int i = 0; i < len; ++i) {
    printf(" %02x", data[i]);
    if ((i + 1) % 16 == 0) {
      printf("\n");
    }
  }
  printf("\n");
}
#else
#include <vector.h>
#include <algorithm.h>
#include <pair.h>
#endif

#ifndef DEBUG
#define DEBUG false
#endif

#define dprintf(...) \
  if (DEBUG) { \
    printf(__VA_ARGS__); \
  }

uint32_t read_ULEB128(const uint8_t*& cur, const uint8_t* end);
int32_t read_SLEB128(const uint8_t*& cur, const uint8_t* end);
uint8_t read_uint8(const uint8_t*& cur, const uint8_t* end);
uint16_t read_uint16(const uint8_t*& cur, const uint8_t* end);
uint32_t read_uint32(const uint8_t*& cur, const uint8_t* end);
const char* read_str(const uint8_t*& cur, const uint8_t* end);

#define FOR_EACH_DW_TAG(_) \
  _(array_type, 0x01) \
  _(class_type, 0x02) \
  _(enumeration_type, 0x04) \
  _(formal_parameter, 0x05) \
  _(label, 0x0a) \
  _(lexical_block, 0x0b) \
  _(member, 0x0d) \
  _(pointer_type, 0x0f) \
  _(reference_type, 0x10) \
  _(compile_unit, 0x11) \
  _(structure_type, 0x13) \
  _(subroutine_type, 0x15) \
  _(typedef, 0x16) \
  _(union_type, 0x17) \
  _(unspecified_parameters, 0x18) \
  _(inheritance, 0x1c) \
  _(subrange_type, 0x21) \
  _(base_type, 0x24) \
  _(const_type, 0x26) \
  _(enumerator, 0x28) \
  _(subprogram, 0x2e) \
  _(template_type_parameter, 0x2f) \
  _(variable, 0x34) \
  _(volatile_type, 0x35) \
  _(namespace, 0x39) \
  _(rvalue_reference_type, 0x42) 

#define FOR_EACH_DW_AT(_) \
  _(sibling, 0x01) \
  _(location, 0x02) \
  _(name, 0x03) \
  _(byte_size, 0x0b) \
  _(bit_size, 0x0d) \
  _(stmt_list, 0x10) \
  _(low_pc, 0x11) \
  _(high_pc, 0x12 /* it actually encodes the number of bytes */) \
  _(language, 0x13) \
  _(comp_dir, 0x1b) \
  _(const_value, 0x1c) \
  _(inline, 0x20) \
  _(producer, 0x25) \
  _(prototyped, 0x27) \
  _(upper_bound, 0x2f) \
  _(abstract_origin, 0x31) \
  _(accessibility, 0x32) \
  _(artificial, 0x34) \
  _(data_member_location, 0x38) \
  _(decl_column, 0x39) \
  _(decl_file, 0x3a) \
  _(decl_line, 0x3b) \
  _(declaration, 0x3c) \
  _(encoding, 0x3e) \
  _(external, 0x3f) \
  _(frame_base, 0x40) \
  _(specification, 0x47) \
  _(type, 0x49) \
  _(ranges, 0x55) \
  _(explicit, 0x63) \
  _(object_pointer, 0x64) \
  _(data_bit_offset, 0x6b) \
  _(const_expr, 0x6c) \
  _(enum_class, 0x6d) \
  _(linkage_name, 0x6e) \
  _(call_all_calls, 0x7a) \
  _(call_all_tail_calls, 0x7c) \
  _(alignment, 0x88) \
  _(deleted, 0x8a) \
  _(defaulted, 0x8b)

#define FOR_EACH_DW_FORM(_) \
  _(addr, 0x01) \
  _(data2, 0x05) \
  _(data4, 0x06) \
  _(string, 0x08) \
  _(data1, 0x0b) \
  _(strp, 0x0e) \
  _(ref4, 0x13) \
  _(sec_offset, 0x17) \
  _(exprloc, 0x18) \
  _(flag_present, 0x19) \
  _(implicit_const, 0x21) \
  _(line_strp, 0x1f)

// standard line number program opcode
#define FOR_EACH_DW_LNS(_) \
  _(copy, 1) \
  _(advance_pc, 2) \
  _(advance_line, 3) \
  _(set_file, 4) \
  _(set_column, 5) \
  _(negate_stmt, 6) \
  _(const_add_pc, 8)

// extended line number program opcode
#define FOR_EACH_DW_LNE(_) \
  _(end_sequence, 1) \
  _(set_address, 2) \
  _(set_discriminator, 4)


enum EnumDW_TAG {
#define ENUM(name, val) DW_TAG_ ## name = val,
FOR_EACH_DW_TAG(ENUM)
#undef ENUM
};

const char* dwtag2str(uint32_t tag);

enum EnumDW_AT {
#define ENUM(name, val) DW_AT_ ## name = val,
FOR_EACH_DW_AT(ENUM)
#undef ENUM
};

const char* dwat2str(uint32_t attr);

enum EnumDW_FORM {
#define ENUM(name, val) DW_FORM_ ## name = val,
FOR_EACH_DW_FORM(ENUM)
#undef ENUM
};

const char* dwform2str(uint32_t form);

enum EnumDW_LNS {
#define ENUM(name, val) DW_LNS_ ## name = val,
FOR_EACH_DW_LNS(ENUM)
#undef ENUM
};

const char* dwlns2str(uint32_t opcode);

enum EnumDW_LNE {
#define ENUM(name, val) DW_LNE_ ## name = val,
FOR_EACH_DW_LNE(ENUM)
#undef ENUM
};

const char* dwlne2str(uint32_t opcode);

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
    // TODO implement emplace_back
    #ifdef HOST_OS
    attr_form_pairs_.emplace_back(attr, form);
    #else
    attr_form_pairs_.push_back(make_pair(attr, form));
    #endif
  }

  uint32_t tag() const { return tag_; };

  void dump() const {
    // TODO %lu not supported yet
    #ifdef HOST_OS
    printf("code %d, tag %s, has_child %d, #pairs %lu\n", code_, dwtag2str(tag_), has_child_, attr_form_pairs_.size());
    #else
    printf("code %d, tag %s, has_child %d, #pairs %d\n", code_, dwtag2str(tag_), has_child_, attr_form_pairs_.size());
    #endif
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
    // hexdump(debug_abbrev_buf, debug_abbrev_nbytes);
  
    const uint8_t* cur = debug_abbrev_buf, *end = debug_abbrev_buf + debug_abbrev_nbytes;
    tables_.push_back(make_pair(0, vector<AbbrevEntry>()));
    vector<AbbrevEntry>* pentries = &(tables_.back().second);
    while (cur < end) {
      uint32_t abbrev_code = read_ULEB128(cur, end);
      if (abbrev_code == 0) {
        dprintf("Got a 0 abbrev_code. Next 0x%lx\n", cur - debug_abbrev_buf);
        tables_.push_back(make_pair((int) (cur - debug_abbrev_buf), vector<AbbrevEntry>()));
        pentries = &(tables_.back().second);
        continue;
      }
      // make sure no duplicate
      assert(find(*pentries, abbrev_code) == NULL);
      uint32_t tag = read_ULEB128(cur, end);
      bool has_child = read_uint8(cur, end);
   
      // TODO: implement emplace_back
      #ifdef HOST_OS
      pentries->emplace_back(abbrev_code, tag, has_child);
      #else
      pentries->push_back(AbbrevEntry(abbrev_code, tag, has_child));
      #endif
      dprintf("Got abbrev code %d, tag %s, has_child %d\n", abbrev_code, dwtag2str(tag), has_child);

      AbbrevEntry& entry = pentries->back();
  
      while (1) {
        uint32_t attr = read_ULEB128(cur, end);
        uint32_t form = read_ULEB128(cur, end);
        if (form == DW_FORM_implicit_const) {
          // DW_FORM_implicit_const contains the const value in the abbrev table.
          // TODO: don't ignore it!
          auto imp_const = read_SLEB128(cur, end);
          dprintf("  implicit const: %d\n", imp_const);
        }
        if (attr == 0) {
          dprintf("  Got 0 attr\n");
          if (form != 0) {
            printf("Got an unexpected form %d\n", form);
          }
          assert(form == 0);
          break;
        }
        dprintf("  Got attr %s, form %s\n", dwat2str(attr), dwform2str(form));
        entry.add_pair(attr, form);
      }
    }
    assert(cur == end);
  }

  void dump() const {
    for (const auto& off_tbl : tables_) {
      // TODO %lu is not supported yet
      #ifdef HOST_OS
      dprintf("Got %lu abbrev entries for offset 0x%x:\n", off_tbl.second.size(), off_tbl.first);
      #else
      dprintf("Got %d abbrev entries for offset 0x%x:\n", off_tbl.second.size(), off_tbl.first);
      #endif

      if (DEBUG) {
        for (const auto& entry : off_tbl.second) {
          entry.dump();
        }
      }
    }
  }

  /*
   * XXX use hash table.
   */
  const AbbrevEntry* find(const vector<AbbrevEntry>& entries, uint32_t code) const {
    for (const auto& entry : entries) {
      if (entry.code_ == code) {
        return &entry;
      }
    }
    return nullptr;
  }

  const vector<AbbrevEntry>* find_by_off(uint32_t off) const {
    for (const auto& kv : tables_) {
      if (kv.first == off) {
        return &kv.second;
      }
    }
    return nullptr;
  }

 private:
  // first member of pair is offset
  vector<pair<int, vector<AbbrevEntry>>> tables_;
};


struct FunctionEntry {
  const char* name;
  uint32_t start;
  uint32_t len;
};

struct LineNoEntry {
 public:
  LineNoEntry(uint32_t _addr, const char* _file_name, uint32_t _lineno)
    : addr(_addr), file_name(_file_name), lineno(_lineno) {
  }

  void dump() const {
    dprintf("  0x%x -> %s:%d\n", addr, file_name, lineno);
  }

  uint32_t addr;
  const char* file_name;
  uint32_t lineno;

  bool operator<(const LineNoEntry& rhs) const {
    const auto& lhs = *this;
    return lhs.addr < rhs.addr;
  }
};

class DwarfContext {
 public:
  explicit DwarfContext(uint8_t* elfbuf, bool do_init=true) : elfbuf_(elfbuf), initialized(false) {
    if (do_init) {
      init();
    }
  }

  void parse_elf(uint8_t* elfbuf);

  void init() {
    assert(!initialized);
    initialized = true;
    parse_elf(elfbuf_);
  }

  const FunctionEntry* find_func_entry_by_addr(uint32_t addr) {
    for (const auto& entry : function_entries_) {
      if (addr >= entry.start && addr < entry.start + entry.len) {
        return &entry;
      }
    }
    return nullptr;
  }

  /*
   * Binary search to find the last entry with entry.addr <= addr
   */
  const LineNoEntry* find_lineno_entry_by_addr(uint32_t addr) {
    int s = 0, e = lntab_.size() - 1, m;
    while (s <= e) {
      m = s + (e - s) / 2;
      if (lntab_[m].addr <= addr) {
        s = m + 1;
      } else {
        e = m - 1;
      }
    }
    if (e >= 0 && e < lntab_.size()) {
      return &lntab_[e];
    } else {
      return nullptr;
    }
  }

  void parse_abbrev_table(const uint8_t* debug_abbrev_buf, int debug_abbrev_nbytes) {
    abbrev_table_.parse(debug_abbrev_buf, debug_abbrev_nbytes);
    abbrev_table_.dump();
  }
  void parse_debug_info(const uint8_t* debug_info_buf, int debug_info_nbytes);
  void parse_debug_info_for_one_abbrev_off(uint32_t unit_offset, const uint8_t* start, const uint8_t* cur, const uint8_t* end);
  void parse_debug_line(const uint8_t* debug_line_buf, int debug_line_nbytes);
  // parse for one line number program
  void parse_debug_line_one_program(const uint8_t* start, const uint8_t* cur, const uint8_t* end);
 private:
  /*
   * If the previous entry has the same addr, overwrite it with the new entry.
   */
  void add_lntab_entry(uint32_t addr, const char* file_name, uint32_t ln) {
    if (addr == 0) {
      // ignore addr == 0 for now
      return;
    }
    if (lntab_.size() > 0 && lntab_.back().addr == addr) {
      lntab_.back().file_name = file_name;
      lntab_.back().lineno = ln;
    } else {
      // TODO don't support emplace_back yet
      #ifdef HOST_OS
      lntab_.emplace_back(addr, file_name, ln);
      #else
      lntab_.push_back(LineNoEntry(addr, file_name, ln));
      #endif
    }
  }

  /*
   * Sort is necessary. Check line number program snippet as follows:
     [0x000004d5]  Advance PC by 2 to 0x114b5b
     [0x000004d7]  Extended opcode 1: End of Sequence

     [0x000004da]  Set File Name to entry 2 in the File Name Table
     [0x000004dc]  Set column to 51
     [0x000004de]  Extended opcode 2: set Address to 0x100816
     [0x000004e5]  Advance Line by 21 to 22
   * 
   * After setting addr to 0x114b5b, the addr will be changed to
   * 0x100816 next.
   */
  void postprocess_lntab() {
    sort(lntab_.begin(), lntab_.end());

    for (int i = 0; i + 1 < lntab_.size(); ++i) {
      const auto& lhs = lntab_[i];
      const auto& rhs = lntab_[i + 1];

      // dump for entries with equal addresses
      if (lhs.addr >= rhs.addr) {
        lhs.dump();
        rhs.dump();
      }

      // there are entries with equal addresses. We should have
      // a better way to dedup them.
      assert(lhs.addr <= rhs.addr);
    }
  }

  void dump_lntab() {
    // TODO sos does not support %lu yet
    #ifdef HOST_OS
    printf("The lntab contains %lu entries\n", lntab_.size());
    #else
    printf("The lntab contains %d entries\n", lntab_.size());
    #endif
    if (DEBUG) {
      for (auto& ent : lntab_) {
        ent.dump();
      }
    }
  }

  void register_func_name(uint32_t offset, const char *fn_name) {
    offset_to_func_name_.push_back(make_pair(offset, fn_name));
  }

  // TODO avoid doing linear scan
  const char* find_func_name_by_spec(uint32_t spec) {
    for (auto& kv : offset_to_func_name_) {
      if (spec == kv.first) {
        return kv.second;
      }
    }
    return nullptr; 
  }

  AbbrevTable abbrev_table_;
  uint8_t* debug_str_buf_;
  uint32_t debug_str_nbytes_;
  uint8_t* debug_line_str_buf_;
  uint32_t debug_line_str_nbytes_;
  vector<FunctionEntry> function_entries_;
  vector<LineNoEntry> lntab_;

  /*
   * A class method may have 2 DW_TAG_subprogram entries.
   * The first entry is for the declaration and contains the method name;
   * the second entry is for the definition and contains low/high pc.
   * But the second entry may not contains the method name but refer
   * to the first entry using its offset with DW_AT_specification .
   * Refer to this SO post for more details:
   *   https://stackoverflow.com/questions/37780465/c-class-methods-do-not-have-an-address-range-in-dwarf-info
   */
  vector<pair<uint32_t, const char*>> offset_to_func_name_;

  uint8_t* elfbuf_;
 public:
  bool initialized;
};
