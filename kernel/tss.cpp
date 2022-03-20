#include <kernel/tss.h>
#include <kernel/asm_util.h>
#include <kernel/idt.h>
#include <stdint.h>

struct task_state_segment {
  uint16_t previous_task_link; uint16_t padding_previous_task_link;
  uint32_t esp0;
  uint16_t ss0; uint16_t padding_ss0;
  uint32_t esp1;
  uint16_t ss1; uint16_t padding_ss1;
  uint32_t esp2;
  uint16_t ss2; uint16_t padding_ss2;
  uint32_t cr3;
  uint32_t eip;
  uint32_t eflags;
  uint32_t eax;
  uint32_t ecx;
  uint32_t edx;
  uint32_t ebx;
  uint32_t esp;
  uint32_t ebp;
  uint32_t esi;
  uint32_t ebi;
  uint16_t es; uint16_t padding_es;
  uint16_t cs; uint16_t padding_cs;
  uint16_t ss; uint16_t padding_ss;
  uint16_t ds; uint16_t padding_ds;
  uint16_t fs; uint16_t padding_fs;
  uint16_t gs; uint16_t padding_gs;
  uint32_t ldt;
  uint32_t iomap;
} tss;
static_assert(sizeof(tss) == 104);

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
  extern char kernel_stack_top[];
  tss.ss0 = KERNEL_DATA_SEG;
  tss.esp0 = (uint32_t) kernel_stack_top;
  tss.ss1 = KERNEL_DATA_SEG;
  tss.esp1 = (uint32_t) kernel_stack_top;
  tss.ss2 = KERNEL_DATA_SEG;
  tss.esp2 = (uint32_t) kernel_stack_top;
  tss_segment_desc = segment_descriptor((uint32_t) &tss, sizeof(tss) - 1, 9 /*type*/, 0 /*granularity*/);
  asm_load_tr();
}
