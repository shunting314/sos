#include <string.h>
#include <stdlib.h>

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

int memcmp(const void* _s1, const void *_s2, int n) {
  const uint8_t* s1 = (const uint8_t*) _s1;
  const uint8_t* s2 = (const uint8_t*) _s2;
  assert(n >= 0);

  for (int i = 0; i < n; ++i) {
    if (*s1 != *s2) {
      return (*s1) - (*s2);
    }
    ++s1;
    ++s2;
  }
  return 0;
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
    // don't compare beyond '\0'
    if (!*s) {
      return 0;
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

char *strcpy(char *dst, const char *src) {
	int i = 0;
	do {
		dst[i] = src[i];
	} while (dst[i++]);
	return dst;
}

char *strncpy(char* dst, const char *src, int n) {
  int i;
  for (i = 0; i < n && src[i]; ++i) {
    dst[i] = src[i];
  }
  while (i < n) {
    dst[i++] = '\0';
  }
  return dst;
}

char *strdup(const char* s) {
  assert(s);
  int len = strlen(s);
  char* dst = malloc(len + 1);
  memmove(dst, s, len + 1);
  return dst;
}

char *strndup(const char* s, int len) {
  char* dst = (char*) malloc(len + 1);
  memmove(dst, s, len);
  dst[len] = '\0';
  return dst;
}
