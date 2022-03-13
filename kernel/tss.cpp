#include <kernel/tss.h>
#include <kernel/asm_util.h>
#include <stdint.h>

struct task_state_segment {
  uint32_t dummy[104 / 4];
} tss;

struct segment_descriptor {
  uint32_t lim_low : 16;
  uint32_t base_low : 16;
  uint32_t base_mid : 8;
  uint32_t type : 4;
  uint32_t reserved : 1;
  uint32_t dpl : 2;
  uint32_t present : 1;
  uint32_t lim_high : 4;
  uint32_t avl : 1;
  uint32_t reserved_2 : 2;
  uint32_t granularity : 1;
  uint32_t base_high : 8;

  segment_descriptor(uint32_t base, uint32_t lim, uint32_t type, bool gran) {
    granularity = gran;
    if (gran) {
      assert(lim & 0xFFF == 0xFFF);
      lim >>= 12;
    }
    lim_low = (lim & 0xFFFF);
    lim_high = ((lim >> 16) & 0xF);
    base_low = (base & 0xFFFF);
    base_mid = ((base >> 16) & 0xFF);
    base_high = ((base >> 24) & 0xFF);
    this->type = type;
    reserved = 0;
    dpl = 0;
    present = 1;
    avl = 0;
    reserved_2 = 0;
  }
};

static_assert(sizeof(segment_descriptor) == 8);

extern segment_descriptor tss_segment_desc;

void setup_tss() {
  tss_segment_desc = segment_descriptor((uint32_t) &tss, sizeof(tss) - 1, 9 /*type*/, 0 /*granularity*/);
  asm_load_tr();
}
