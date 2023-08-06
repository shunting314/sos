#ifndef STDIO_H
#define STDIO_H

#ifdef __cplusplus
extern "C" {
#endif

int printf(const char* fmt, ...);
int puts(const char *s);
int putchar(int ch);

// this is only for user mode
#define FILE void
char* fgets(char* s, int size, FILE* stream);

#ifdef __cplusplus
}
#endif

#endif
