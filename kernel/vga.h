#ifndef KERNEL_VGA_H
#define KERNEL_VGA_H

#ifdef __cplusplus
extern "C" {
#endif

void show_palette();
void vga_clear();
void vga_putchar(char ch);

// simple animate in text mode
void show_flashing_digits();
void show_running_curve();
void place_cursor();

void change_global_color(int color_code);

#ifdef __cplusplus
}
#endif

#endif
