#include <string.h>

// TODO add unit test
void *memmove(void *dst, void *src, int n) {
  if (n <= 0) {
    return dst;
  }
  if (dst < src) {
    char *_dst = dst;
    char *_src = src;
    for (int i = 0; i < n; ++i, ++_src, ++_dst) {
      *_dst = *_src;
    }
  } else if (dst > src) {
    char *_dst = dst + n - 1;
    char *_src = src + n - 1;
    for (int i = 0; i < n; ++i, --_src, --_dst) {
      *_dst = *_src;
    }
  }
  return dst;
}
