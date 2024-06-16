#pragma once

// map IRQ to interrupt numbers in the range of [0x20, 0x30)
void pic_remap();

void pic_send_eoi(int irq);
