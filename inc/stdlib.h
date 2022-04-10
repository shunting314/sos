#ifndef STDLIB_H
#define STDLIB_H

int abs(int v) {
  return v > 0 ? v : -v;
}

int atoi(const char* s) {
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

#endif
