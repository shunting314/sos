#include <kernel/pit.h>
#include <assert.h>

/*
 * The 8253 PIT oscillator runs at 1.193182 MHz. To let channel 0 generate a
 * signal every 10 milliseconds, we need set the frequency to 100 HZ.
 * So the dividor we need is: 1.193182 MHz / 100 HZ ~= 11932 = 0x2e9c
 */
asm(R"(
.global init_pit
init_pit:
  movb $0x34, %al # 0011_0100b
  outb %al, $0x43
  movb $0x9c, %al
  outb %al, $0x40
  movb $0x2e, %al
  outb %al, $0x40
  ret
)");
