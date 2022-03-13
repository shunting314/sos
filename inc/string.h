#ifndef STRING_H
#define STRING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void *memmove(void *dst, void *src, int n);
void *memset(void *va, int c, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif
