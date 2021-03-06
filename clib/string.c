#include <string.h>

// TODO add unit test
void *memmove(void *dst, const void *src, int n) {
  if (n <= 0) {
    return dst;
  }
  if (dst < src) {
    char *_dst = dst;
    const char *_src = src;
    for (int i = 0; i < n; ++i, ++_src, ++_dst) {
      *_dst = *_src;
    }
  } else if (dst > src) {
    char *_dst = dst + n - 1;
    const char *_src = src + n - 1;
    for (int i = 0; i < n; ++i, --_src, --_dst) {
      *_dst = *_src;
    }
  }
  return dst;
}

void *memset(void *va, int c, uint32_t size) {
  char *ca = (char *) va;
  for (int i = 0; i < size; ++i) {
    ca[i] = c;
  }
  return va; 
}

int strcmp(const char *s, const char *t) {
  while (*s && *t && *s == *t) {
    ++s;
    ++t;
  }
  return (*s) - (*t);
}

int strncmp(const char *s, const char *t, int len) {
  assert(len >= 0);

  for (int i = 0; i < len; ++i) {
    if (*s != *t) {
      return (*s) - (*t);
    }
    ++s;
    ++t;
  }
  return 0;
}

int strlen(const char* s) {
  int len = 0;
  while (*s++) {
    ++len;
  }
  return len;
}
