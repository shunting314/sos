#ifndef STRING_H
#define STRING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void *memmove(void *dst, const void *src, int n);
void *memset(void *va, int c, uint32_t size);
int strcmp(const char* s, const char *t);
int strncmp(const char* s, const char *t, int len);
int strlen(const char *s);

#ifdef __cplusplus
}
#endif

#endif
