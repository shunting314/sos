#ifndef STRING_H
#define STRING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void *memmove(void *dst, const void *src, int n);
void *memset(void *va, int c, uint32_t size);
int memcmp(const void *s1, const void *s2, int n);
int strcmp(const char* s, const char *t);
int strncmp(const char* s, const char *t, int len);
int strlen(const char *s);
char *strcpy(char *dst, const char *src);

#ifdef __cplusplus
}
#endif

#endif
