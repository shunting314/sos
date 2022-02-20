#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BASE ((uint16_t*) 0xB8000)
#define NROW 25
#define NCOL 80
#define GRAY_ON_BLACK 0x07

int vga_to_loc(int r, int c) {
  return r * NCOL + c;
}

void vga_set_char(int r, int c, int ch) {
  BASE[vga_to_loc(r, c)] = ch; 
}

int _cursor_loc = 0;

void vga_putchar(char ch) {
  if (ch == '\r') {
    _cursor_loc -= (_cursor_loc % NCOL);
  } else if (ch == '\n') {
    // treated as '\r' plus '\n' here
    _cursor_loc -= (_cursor_loc % NCOL);
    _cursor_loc += NCOL;
  } else if (ch == '\t') {
    // simply translate '\t' to 4 ' '. Do something better?
    for (int i = 0; i < 4; ++i) {
      vga_putchar(' ');
    }
    return;
  } else {
    BASE[_cursor_loc] = (ch) | (GRAY_ON_BLACK << 8);
    _cursor_loc++;
  }

  // scroll if needed
  if (_cursor_loc >= NROW * NCOL) {
    // We need scroll at most 1 row
    memmove(BASE, BASE + NCOL, (NROW - 1) * NCOL * sizeof(uint16_t));
    for (int c = 0; c < NCOL; ++c) {
      vga_set_char(NROW - 1, c, ' ' | (GRAY_ON_BLACK << 8));
    }
    _cursor_loc -= NCOL;
  }
}

void vga_clear() {
  for (int i = 0; i < NROW; ++i) {
    for (int j = 0; j < NCOL; ++j) {
      vga_set_char(i, j, ' ' | (GRAY_ON_BLACK << 8));
    }
  }
}

/*
 * Draw a screen that shows how colors are encoded.
 */
void show_palette() {
  int startrow = 2;
  int startcol = 2;
  for (int i = 0; i < 16; ++i) {
    for (int j = 0; j < 16; ++j) {
      vga_set_char(startrow + i, startcol + j, 'X' | (((i << 4) | j) << 8));
    }
  }
}

void _pause() {
  for (int i = 0; i < 10000000; ++i) {
  }
}

void show_flashing_digits() {
  int off = 0;
  while (1) {
    for (int i = 0; i < NROW * NCOL; ++i) {
      char ch = (i % 10) + '0';
      char col = (i + off) % 16;
      BASE[i] = (ch | (col << 8));
    }
    off++;
    _pause();
  }
}

/*
 * I'd rather drawing a sin curve but sin function is not available right now.
 */
void show_running_curve() {
  int off = 0;
  int rowoff = 5;
  while (1) {
    vga_clear();
    for (int c = 0; c < NCOL; ++c) {
      int r = abs((c + off) % 20 - 10) + rowoff;
      vga_set_char(r, c, 'x' | 0x0600);
    }
    off++;
    _pause();
  }
}
