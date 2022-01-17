#ifndef KERNEL_VGA_H
#define KERNEL_VGA_H

void show_palette();
void vga_clear();

// simple animate in text mode
void show_flashing_digits();
void show_running_curve();

#endif
