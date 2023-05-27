#ifndef STDLIB_H
#define STDLIB_H

#include <stdint.h>

static inline int abs(int v) {
  return v > 0 ? v : -v;
}

static inline int atoi(const char* s) {
  int ret = 0;
  int sign = 1;
  if (*s == '+') {
    ++s;
  } else if (*s == '-') {
    sign = -1;
    ++s;
  }
  int base = 10;
  if (*s == '0') {
    ++s;
    if (*s == 'x' || *s == 'X') {
      base = 16;
      ++s;
    } else {
      base = 8;
    }
  }
  while (*s) {
    int d = -1;
    if (*s >= '0' && *s <= '9') {
      d = *s - '0';
    } else if (*s >= 'A' && *s <= 'F') {
      d = *s - 'A' + 10;
    } else if (*s >= 'a' && *s <= 'f') {
      d = *s - 'a' + 10;
    }
    if (d < 0 || d >= base) {
      break;
    }
    ret = ret * base + d;
    ++s;
  }
  return sign * ret;
}

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define ROUND_UP(s, base) ((s + base - 1) / base * base)

// return -1 for non hex char
static inline int hexchar2int(char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  } else if (ch >= 'A' && ch <= 'F') {
    return ch - 'A' + 10;
  } else if (ch >= 'a' && ch <= 'f') {
    return ch - 'a' + 10;
  } else {
    return -1;
  }
}

static inline void hexdump(const uint8_t *data, int len) {
  for (int i = 0; i < len; ++i) {
    // TODO implememt padding for printf so we can pad 0 for single digit hex numbers here
    printf(" %x", data[i]);
    if ((i + 1) % 16 == 0) {
      printf("\n");
    }
  }
  printf("\n");
}

// memory management
void* malloc(uint32_t nbytes);
void* realloc(void* ptr, uint32_t sz);
void free(void* ptr);

#endif
